// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2021 Sartura */
#include "../ebpf_types.h"
#include "vmlinux.h"
#include "../../../include/secDetector_topic.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define S_IFMT 00170000
#define S_IFREG 0100000
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

#define O_CREAT 100
#define LOOKUP_CREATE 0x0200
#define FMODE_CREATED 0x100000

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct
{
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 1024 * 1024);
} rb SEC(".maps");

const volatile int secdetector_pid = -1;

#define RETURN_IF_OURSELF(retval) \
do {   \
	if (secdetector_pid == bpf_get_current_pid_tgid() >> 32) \
		return retval;  \
} while(0)

#define RETURN_ZERO_IF_OURSELF() RETURN_IF_OURSELF(0)

static inline u32 get_task_sid(struct task_struct *task)
{
	struct pid *pid;
	u32 sid = 0;

	pid = BPF_CORE_READ(task, signal, pids[PIDTYPE_SID]);
	if (!pid)
		return 0;
	sid = BPF_CORE_READ(pid, numbers[0].nr);
	return sid;
}

static struct task_struct *find_init_task()
{
	int i = 0;
	struct task_struct *task;
	struct task_struct *init_task = NULL;
	task = (struct task_struct *)bpf_get_current_task();

	for (i = 0; i < 64; i++)
	{
		task = BPF_CORE_READ(task, real_parent);
		if (!task || init_task == task)
		{
			break;
		}

		init_task = task;
	}
	return task;
}

static struct pid_namespace *pid_ns(struct task_struct *task)
{
	struct pid *pid = NULL;
	u32 level = 0;
	pid = BPF_CORE_READ(task, thread_pid);
	if (!pid)
		return NULL;

	return BPF_CORE_READ(pid, numbers[0].ns);
}

static void get_common_info(struct ebpf_event *e)
{
	struct task_struct *parent = NULL;
	struct task_struct *task = NULL;

	e->timestamp = bpf_ktime_get_ns();
	e->pid = bpf_get_current_pid_tgid() >> 32;
	e->pgid = e->tgid = bpf_get_current_pid_tgid() >> 32;
	e->uid = bpf_get_current_uid_gid();
	e->gid = bpf_get_current_uid_gid() >> 32;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
	/*
	 * exe path is diffcult to get in ebpf, we can get it from userspace
	 */
	bpf_get_current_comm(&e->exe, sizeof(e->exe));

	task = (struct task_struct *)bpf_get_current_task();
	parent = (struct task_struct *)BPF_CORE_READ(task, real_parent);

	e->ppid = BPF_CORE_READ(parent, pid);
	e->sid = get_task_sid(task);
	e->pns = BPF_CORE_READ(pid_ns(task), ns.inum);
	e->root_pns = BPF_CORE_READ(pid_ns(find_init_task()), ns.inum);
	BPF_CORE_READ_INTO(&e->pcomm, parent, real_parent, comm);
	BPF_CORE_READ_INTO(&e->nodename, task, nsproxy, uts_ns, name.nodename);
}

SEC("fexit/do_filp_open")
int BPF_PROG(do_filp_open_exit, int dfd, struct filename *pathname, const struct open_flags *op, struct file *ret_file)
{
	struct ebpf_event *e = NULL;
	RETURN_ZERO_IF_OURSELF();

	if (op && (!(op->open_flag & O_CREAT) || !(op->intent & LOOKUP_CREATE)))
		return 0;
	if (!S_ISREG(ret_file->f_inode->i_mode))
		return 0;
	if (!(ret_file->f_mode & FMODE_CREATED))
		return 0;

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = CREATEFILE;

	struct task_struct *parent = NULL;
	struct task_struct *task = NULL;

	e->timestamp = bpf_ktime_get_ns();
	e->pid = bpf_get_current_pid_tgid() >> 32;
	e->pgid = e->tgid = bpf_get_current_pid_tgid() >> 32;
	e->uid = bpf_get_current_uid_gid();
	e->gid = bpf_get_current_uid_gid() >> 32;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
	/*
	 * exe path is diffcult to get in ebpf, we can get it from userspace
	 */
	bpf_get_current_comm(&e->exe, sizeof(e->exe));

	task = (struct task_struct *)bpf_get_current_task();
	parent = (struct task_struct *)BPF_CORE_READ(task, real_parent);

	e->ppid = BPF_CORE_READ(parent, pid);
	e->sid = get_task_sid(task);
	e->pns = BPF_CORE_READ(pid_ns(task), ns.inum);
	e->root_pns = BPF_CORE_READ(pid_ns(find_init_task()), ns.inum);
	BPF_CORE_READ_INTO(&e->pcomm, parent, real_parent, comm);
	BPF_CORE_READ_INTO(&e->nodename, task, nsproxy, uts_ns, name.nodename);
	//get_common_info(e);
	__builtin_memcpy(e->event_name, "createfile", sizeof("createfile"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, pathname->name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fexit/vfs_write")
int BPF_PROG(fexit_vfs_write, struct file *file, const char *buf, size_t count, long long *pos, ssize_t ret)
{
	struct ebpf_event *e = NULL;
	if (ret <= 0)
		return 0;

	RETURN_ZERO_IF_OURSELF();

	if (!S_ISREG(file->f_inode->i_mode))
		return 0;

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = WRITEFILE;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "writefile", sizeof("writefile"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, file->f_path.dentry->d_name.name);
	//bpf_d_path(&file->f_path, e->file_info.filename, MAX_TEXT_SIZE);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fexit/vfs_unlink")
int BPF_PROG(fexit_vfs_unlink, struct inode *dir, struct dentry *dentry, struct inode **delegated_inode, int ret)
{
	struct ebpf_event *e = NULL;

	if (ret != 0)
		return 0;

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = DELFILE;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "delfile", sizeof("delfile"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, dentry->d_name.name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fexit/vfs_read")
int BPF_PROG(fexit_vfs_read, struct file *file, char *buf, size_t count, long long *pos, ssize_t ret)
{
	struct ebpf_event *e = NULL;

	if (ret != 0)
		return 0;

	RETURN_ZERO_IF_OURSELF();

	if (!S_ISREG(file->f_inode->i_mode))
		return 0;
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;
	e->type = READFILE;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "readfile", sizeof("readfile"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, file->f_path.dentry->d_name.name);
	//bpf_d_path(&file->f_path, e->file_info.filename, MAX_TEXT_SIZE);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fentry/vfs_utimes")
int BPF_PROG(fexit_vfs_utimes, const struct path *path, struct timespec64 *times)
{
	struct ebpf_event *e = NULL;
	char name[] = "time";

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, path->dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fexit/chmod_common")
int BPF_PROG(fexit_chmod_common, const struct path *path, umode_t mode, int ret)
{
	struct ebpf_event *e = NULL;
	char name[] = "chmod";

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, path->dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fexit/chown_common")
int BPF_PROG(fexit_chown_common, const struct path *path, uid_t user, gid_t group, int ret)
{
	struct ebpf_event *e = NULL;
	char name[] = "chown";

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, path->dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fentry/__vfs_setxattr_noperm")
int BPF_PROG(fentry__vfs_setxattr_noperm, struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
{
	struct ebpf_event *e = NULL;

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fentry/__vfs_removexattr_locked")
int BPF_PROG(fentry__vfs_removexattr_locked, struct dentry *dentry, const char *name, struct inode **delegated_inode)
{
	struct ebpf_event *e = NULL;

	RETURN_ZERO_IF_OURSELF();

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("fentry/vfs_rename")
int BPF_PROG(fentry_vfs_rename, struct inode *old_dir, struct dentry *old_dentry,
	struct inode *new_dir, struct dentry *new_dentry, struct inode **delegated_inode,
	unsigned int flags)
{
	struct ebpf_event *e = NULL;
	char name[] = "rename";

	RETURN_ZERO_IF_OURSELF();
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->type = SETFILEATTR;
	get_common_info(e);
	__builtin_memcpy(e->event_name, "setfileattr", sizeof("setfileattr"));
	bpf_probe_read(e->file_info.filename, MAX_TEXT_SIZE, old_dentry->d_name.name);
	bpf_probe_read_str(e->file_info.name, MAX_TEXT_SIZE, name);
	bpf_probe_read(e->file_info.value, MAX_TEXT_SIZE, new_dentry->d_name.name);
	bpf_probe_read(e->file_info.old_value, MAX_TEXT_SIZE, old_dentry->d_name.name);
	bpf_ringbuf_submit(e, 0);
	return 0;
}

