/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: Andrii Nakryiko chenjingwen
 * create: 2023-09-21
 * Description: Derived from the ebpf ringbuffer implementation,
 *              but provides the interface as a device file
 */
#include <linux/err.h>
#include <linux/irq_work.h>
#include <linux/slab.h>
#include <linux/filter.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/kmemleak.h>
#include <linux/fs.h>

#define rb_datasz (PAGE_SIZE << 10) /* 4Mb */
#define rb_mask (rb_datasz - 1)

#define MODULE_DEVICE "secDetector"

#define RINGBUF_CREATE_FLAG_MASK (BPF_F_NUMA_NODE)

/* non-mmap()'able part of ringbuf (everything up to consumer page) */
#define RINGBUF_PGOFF \
	(offsetof(struct ringbuf, consumer_pos) >> PAGE_SHIFT)
/* consumer page and producer page */
#define RINGBUF_POS_PAGES 2

#define RINGBUF_MAX_RECORD_SZ (UINT_MAX/4)

/* Maximum size of ring buffer area is limited by 32-bit page offset within
 * record header, counted in pages. Reserve 8 bits for extensibility, and take
 * into account few extra pages for consumer/producer pages and
 * non-mmap()'able parts. This gives 64GB limit, which seems plenty for single
 * ring buffer.
 */
#define RINGBUF_MAX_DATA_SZ \
	(((1ULL << 24) - RINGBUF_POS_PAGES - RINGBUF_PGOFF) * PAGE_SIZE)

struct ringbuf {
	wait_queue_head_t waitq;
	struct irq_work work;
	u64 mask;
	struct page **pages;
	int nr_pages;
	spinlock_t spinlock ____cacheline_aligned_in_smp;
	/* Consumer and producer counters are put into separate pages to allow
	 * mapping consumer page as r/w, but restrict producer page to r/o.
	 * This protects producer position from being modified by user-space
	 * application and ruining in-kernel position tracking.
	 */
	unsigned long consumer_pos __aligned(PAGE_SIZE);
	unsigned long producer_pos __aligned(PAGE_SIZE);
	char data[] __aligned(PAGE_SIZE);
};

/* 8-byte ring buffer record header structure */
struct ringbuf_hdr {
	u32 len;
	u32 pg_off;
};

static int major;
static struct class *class;
static struct ringbuf *g_rb;
static unsigned long g_isopen;

static struct ringbuf *ringbuf_area_alloc(size_t data_sz, int numa_node)
{
	const gfp_t flags = GFP_KERNEL | __GFP_RETRY_MAYFAIL | __GFP_NOWARN |
			    __GFP_ZERO;
	int nr_meta_pages = RINGBUF_PGOFF + RINGBUF_POS_PAGES;
	int nr_data_pages = data_sz >> PAGE_SHIFT;
	int nr_pages = nr_meta_pages + nr_data_pages;
	struct page **pages, *page;
	struct ringbuf *rb;
	size_t array_size;
	int i;

	/* Each data page is mapped twice to allow "virtual"
	 * continuous read of samples wrapping around the end of ring
	 * buffer area:
	 * ------------------------------------------------------
	 * | meta pages |  real data pages  |  same data pages  |
	 * ------------------------------------------------------
	 * |            | 1 2 3 4 5 6 7 8 9 | 1 2 3 4 5 6 7 8 9 |
	 * ------------------------------------------------------
	 * |            | TA             DA | TA             DA |
	 * ------------------------------------------------------
	 *                               ^^^^^^^
	 *                                  |
	 * Here, no need to worry about special handling of wrapped-around
	 * data due to double-mapped data pages. This works both in kernel and
	 * when mmap()'ed in user-space, simplifying both kernel and
	 * user-space implementations significantly.
	 */
	array_size = (nr_meta_pages + 2 * nr_data_pages) * sizeof(*pages);
	if (array_size > PAGE_SIZE)
		pages = vmalloc_node(array_size, numa_node);
	else
		pages = kmalloc_node(array_size, flags, numa_node);
	if (!pages)
		return NULL;

	for (i = 0; i < nr_pages; i++) {
		page = alloc_pages_node(numa_node, flags, 0);
		if (!page) {
			nr_pages = i;
			goto err_free_pages;
		}
		pages[i] = page;
		if (i >= nr_meta_pages)
			pages[nr_data_pages + i] = page;
	}

	rb = vmap(pages, nr_meta_pages + 2 * nr_data_pages,
		  VM_MAP | VM_USERMAP, PAGE_KERNEL);
	if (rb) {
		kmemleak_not_leak(pages);
		rb->pages = pages;
		rb->nr_pages = nr_pages;
		return rb;
	}

err_free_pages:
	for (i = 0; i < nr_pages; i++)
		__free_page(pages[i]);
	kvfree(pages);
	return NULL;
}

static void ringbuf_notify(struct irq_work *work)
{
	struct ringbuf *rb = container_of(work, struct ringbuf, work);

	wake_up_all(&rb->waitq);
}

static struct ringbuf *ringbuf_alloc(size_t data_sz, int numa_node)
{
	struct ringbuf *rb;

	rb = ringbuf_area_alloc(data_sz, numa_node);
	if (!rb)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&rb->spinlock);
	init_waitqueue_head(&rb->waitq);
	init_irq_work(&rb->work, ringbuf_notify);

	rb->mask = data_sz - 1;
	rb->consumer_pos = 0;
	rb->producer_pos = 0;

	return rb;
}

static void ringbuf_free(struct ringbuf *rb)
{
	/* copy pages pointer and nr_pages to local variable, as we are going
	 * to unmap rb itself with vunmap() below
	 */
	struct page **pages = rb->pages;
	int i, nr_pages = rb->nr_pages;

	vunmap(rb);
	for (i = 0; i < nr_pages; i++)
		__free_page(pages[i]);
	kvfree(pages);
}

static unsigned long ringbuf_avail_data_sz(struct ringbuf *rb)
{
	unsigned long cons_pos, prod_pos;

	cons_pos = smp_load_acquire(&rb->consumer_pos);
	prod_pos = smp_load_acquire(&rb->producer_pos);
	return prod_pos - cons_pos;
}

/* Given pointer to ring buffer record metadata and struct ringbuf itself,
 * calculate offset from record metadata to ring buffer in pages, rounded
 * down. This page offset is stored as part of record metadata and allows to
 * restore struct ringbuf * from record pointer. This page offset is
 * stored at offset 4 of record metadata header.
 */
static size_t ringbuf_rec_pg_off(struct ringbuf *rb,
				     struct ringbuf_hdr *hdr)
{
	return ((void *)hdr - (void *)rb) >> PAGE_SHIFT;
}

/* Given pointer to ring buffer record header, restore pointer to struct
 * ringbuf itself by using page offset stored at offset 4
 */
static struct ringbuf *
ringbuf_restore_from_rec(struct ringbuf_hdr *hdr)
{
	unsigned long addr = (unsigned long)(void *)hdr;
	unsigned long off = (unsigned long)hdr->pg_off << PAGE_SHIFT;

	return (void*)((addr & PAGE_MASK) - off);
}

static void *__ringbuf_reserve(struct ringbuf *rb, u64 size)
{
	unsigned long cons_pos, prod_pos, new_prod_pos, flags;
	u32 len, pg_off;
	struct ringbuf_hdr *hdr;

	if (unlikely(size > RINGBUF_MAX_RECORD_SZ))
		return NULL;

	len = round_up(size + BPF_RINGBUF_HDR_SZ, 8);
	if (len > rb->mask + 1)
		return NULL;

	cons_pos = smp_load_acquire(&rb->consumer_pos);

	if (in_nmi()) {
		if (!spin_trylock_irqsave(&rb->spinlock, flags))
			return NULL;
	} else {
		spin_lock_irqsave(&rb->spinlock, flags);
	}

	prod_pos = rb->producer_pos;
	new_prod_pos = prod_pos + len;

	/* check for out of ringbuf space by ensuring producer position
	 * doesn't advance more than (ringbuf_size - 1) ahead
	 */
	if (new_prod_pos - cons_pos > rb->mask) {
		spin_unlock_irqrestore(&rb->spinlock, flags);
		return NULL;
	}

	hdr = (void *)rb->data + (prod_pos & rb->mask);
	pg_off = ringbuf_rec_pg_off(rb, hdr);
	hdr->len = size | BPF_RINGBUF_BUSY_BIT;
	hdr->pg_off = pg_off;

	/* pairs with consumer's smp_load_acquire() */
	smp_store_release(&rb->producer_pos, new_prod_pos);

	spin_unlock_irqrestore(&rb->spinlock, flags);

	return (void *)hdr + BPF_RINGBUF_HDR_SZ;
}

static void ringbuf_commit(void *sample, u64 flags, bool discard)
{
	unsigned long rec_pos, cons_pos;
	struct ringbuf_hdr *hdr;
	struct ringbuf *rb;
	u32 new_len;

	hdr = sample - BPF_RINGBUF_HDR_SZ;
	rb = ringbuf_restore_from_rec(hdr);
	new_len = hdr->len ^ BPF_RINGBUF_BUSY_BIT;
	if (discard)
		new_len |= BPF_RINGBUF_DISCARD_BIT;

	/* update record header with correct final size prefix */
	xchg(&hdr->len, new_len);

	/* if consumer caught up and is waiting for our record, notify about
	 * new data availability
	 */
	rec_pos = (void *)hdr - (void *)rb->data;
	cons_pos = smp_load_acquire(&rb->consumer_pos) & rb->mask;

	if (flags & BPF_RB_FORCE_WAKEUP)
		irq_work_queue(&rb->work);
	else if (cons_pos == rec_pos && !(flags & BPF_RB_NO_WAKEUP))
		irq_work_queue(&rb->work);
}

static int ringbuffer_mmap(struct file *flip, struct vm_area_struct *vma)
{
	if (vma->vm_flags & VM_WRITE) {
		/* allow writable mapping for the consumer_pos only */
		if (vma->vm_pgoff != 0 || vma->vm_end - vma->vm_start != PAGE_SIZE)
			return -EPERM;
	} else {
		vma->vm_flags &= ~VM_MAYWRITE;
	}
	/* remap_vmalloc_range() checks size and offset */
	return remap_vmalloc_range(vma, g_rb, vma->vm_pgoff + RINGBUF_PGOFF);
}

static __poll_t ringbuffer_poll(struct file *filp, struct poll_table_struct *pts)
{
	poll_wait(filp, &g_rb->waitq, pts);

	if (ringbuf_avail_data_sz(g_rb))
		return EPOLLIN | EPOLLRDNORM;
	return 0;
}

static int ringbuffer_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &g_isopen)) {
		pr_debug("Another process owns this ringbuffer\n");
		return -EBUSY;
	}
	return 0;
}

static int ringbuffer_release(struct inode *inode, struct file *file)
{
	clear_bit(0, &g_isopen);
	return 0;
}

static const struct file_operations dev_fops = {
	.open = ringbuffer_open,
	.release = ringbuffer_release,
	.mmap = ringbuffer_mmap,
	.poll = ringbuffer_poll,
	.owner = THIS_MODULE,
};

static int ringbuf_output(struct ringbuf *rb, const void *data, u64 size, u64 flags)
{
	void *rec;

	if (unlikely(flags & ~(BPF_RB_NO_WAKEUP | BPF_RB_FORCE_WAKEUP)))
		return -EINVAL;

	rec = __ringbuf_reserve(rb, size);
	if (!rec)
		return -EAGAIN;

	memcpy(rec, data, size);
	ringbuf_commit(rec, flags, false /* discard */);
	return 0;
}

int secDetector_ringbuf_output(const void *data, u64 size, u64 flags)
{
	return ringbuf_output(g_rb, data, size, flags);
}

int __init secDetector_ringbuf_dev_init(void)
{
	struct device *class_dev = NULL;
	int ret = 0;

	g_rb = ringbuf_alloc(rb_datasz, 0);
	if (!g_rb)
		return -ENOMEM;

	major = register_chrdev(0, MODULE_DEVICE, &dev_fops);
	if (major < 0) {
		pr_err("unable to get major for dev\n");
		ret = major;
		goto error_free;
	}

	class = class_create(THIS_MODULE, MODULE_DEVICE);
	if (IS_ERR(class)) {
		ret = PTR_ERR(class);
		goto error_class_create;
	}

	class_dev = device_create(class, NULL, MKDEV((unsigned int)major, 1), NULL, MODULE_DEVICE);
	if (unlikely(IS_ERR(class_dev))) {
		ret = PTR_ERR(class_dev);
		goto error_device_create;
	}

	return 0;

error_device_create:
	class_destroy(class);
error_class_create:
	unregister_chrdev(major, MODULE_DEVICE);
error_free:
	ringbuf_free(g_rb);
	g_rb = NULL;
	return ret;
}

void __exit secDetector_ringbuf_dev_exit(void)
{
	device_destroy(class, MKDEV((unsigned int)major, 1));
	class_destroy(class);
	unregister_chrdev(major, MODULE_DEVICE);
	ringbuf_free(g_rb);
}