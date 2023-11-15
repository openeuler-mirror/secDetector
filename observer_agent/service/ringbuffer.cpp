/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 *
 * Author: chenjingwen
 * Create: 2023-09-25
 * Description: secDetector userspace ringbuffer source
 */
#include "ringbuffer.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/bpf.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define rb_datasz (PAGE_SIZE << 10) /* 4Mb */
#define rb_mask (rb_datasz - 1)
#define RINGBUF_DEV "/dev/secDetector"
#define BPF_RINGBUF_HDR_SZ 8

#define READ_ONCE(x) (*(volatile typeof(x) *)&x)
#define WRITE_ONCE(x, v) (*(volatile typeof(x) *)&x) = (v)
#define barrier() asm volatile("" ::: "memory")

#define smp_load_acquire(p)                                                                                            \
    ({                                                                                                                 \
        typeof(*p) ___p = READ_ONCE(*p);                                                                               \
        barrier();                                                                                                     \
        ___p;                                                                                                          \
    })

#define smp_store_release(p, v)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        barrier();                                                                                                     \
        WRITE_ONCE(*p, v);                                                                                             \
    } while (0)

struct secDetector_ringbuffer
{
    unsigned long *consumer_pos;
    unsigned long *producer_pos;
    char *data;
    struct pollfd poll_fd;
    bool inited;
};

static struct secDetector_ringbuffer sec_rb;

static inline int roundup_len(__u32 len)
{
    len <<= 2;
    len >>= 2;
    len += BPF_RINGBUF_HDR_SZ;

    return (len + 7) / 8 * 8;
}

int secDetector_ringbuf_attach(void)
{
    int fd;
    int err;

    fd = open(RINGBUF_DEV, O_RDWR);
    if (fd == -1)
    {
        printf("%s open failed, errno:%d\n", RINGBUF_DEV, errno);
        return -1;
    }

    sec_rb.consumer_pos = (unsigned long *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sec_rb.consumer_pos == MAP_FAILED)
    {
        err = errno;
        printf("failed to map consumer_pos err:%d\n", err);
        close(fd);
        return err;
    }
    sec_rb.producer_pos = (unsigned long *)mmap(NULL, PAGE_SIZE + 2 * rb_datasz, PROT_READ, MAP_SHARED, fd, PAGE_SIZE);
    if (sec_rb.producer_pos == MAP_FAILED)
    {
        err = errno;
        printf("failed to map producer_pos err:%d\n", err);
        close(fd);
        return err;
    }
    sec_rb.data = (char *)sec_rb.producer_pos + PAGE_SIZE;
    sec_rb.poll_fd.fd = fd;
    sec_rb.poll_fd.events = POLLIN;
    sec_rb.inited = true;
    return 0;
}

void secDetector_ringbuf_detach(void)
{
    if (!sec_rb.inited)
        return;

    close(sec_rb.poll_fd.fd);
    sec_rb.poll_fd.fd = -1;
    munmap((void *)sec_rb.consumer_pos, PAGE_SIZE);
    sec_rb.consumer_pos = NULL;
    munmap(sec_rb.producer_pos, PAGE_SIZE + 2 * rb_datasz);
    sec_rb.producer_pos = NULL;

    sec_rb.inited = false;
}

int secDetector_ringbuf_poll(poll_cb cb)
{
    int r;
    bool got_new_data;
    unsigned long cons_pos, prod_pos;
    int *len_ptr, len, err;
    void *sample;
    int64_t cnt = 0;

    r = poll(&sec_rb.poll_fd, 1, -1);

    if (r == -1)
    {
        err = errno;
        printf("poll err:%d\n", err);
        return -1;
    }

    if (r == 0)
    {
        printf("poll timeout\n");
        return -1;
    }
    if (r != 1)
    {
        printf("poll failed, r:%d\n", r);
        return -1;
    }

    cons_pos = smp_load_acquire(sec_rb.consumer_pos);
    do
    {
        got_new_data = false;
        prod_pos = smp_load_acquire(sec_rb.producer_pos);
        while (cons_pos < prod_pos)
        {
            len_ptr = (int32_t *)(sec_rb.data + (cons_pos & rb_mask));
            len = smp_load_acquire(len_ptr);

            /* sample not committed yet, bail out for now */
            if (len & BPF_RINGBUF_BUSY_BIT)
                goto done;

            got_new_data = true;
            cons_pos += roundup_len(len);

            if ((len & BPF_RINGBUF_DISCARD_BIT) == 0)
            {
                sample = (char *)len_ptr + BPF_RINGBUF_HDR_SZ;
                err = cb(sample, len);
                if (err < 0)
                {
                    /* update consumer pos and bail out */
                    smp_store_release(sec_rb.consumer_pos, cons_pos);
                    return err;
                }
                cnt++;
            }

            smp_store_release(sec_rb.consumer_pos, cons_pos);
        }
    } while (got_new_data);
done:

    return 0;
}