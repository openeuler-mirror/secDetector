// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2021 Sartura */
#include "vmlinux.h"
#include "fentry.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 1024 * 1024);
} rb SEC(".maps");

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

	for (i = 0; i < 64; i++) {
		task = BPF_CORE_READ(task, real_parent);
		if (!task || init_task == task) {
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
	e->pid = bpf_get_current_pid_tgid();
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

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
	struct ebpf_event *e = NULL;

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	get_common_info(e);
	strcpy(e->event_name, "sched_process_exec");

	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_exit *ctx)
{
	struct ebpf_event *e = NULL;
	u32 exit_code = 0;

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	get_common_info(e);
	strcpy(e->event_name, "sched_process_exit");
	exit_code = BPF_CORE_READ((struct task_struct *)bpf_get_current_task(), exit_code);
	e->process_info.exit_code = (exit_code >> 8) & 0xff;
	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("tp/sched/sched_process_fork")
int handle_fork(struct trace_event_raw_sched_process_fork *ctx)
{
	struct ebpf_event *e = NULL;

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	get_common_info(e);
	strcpy(e->event_name, "sched_process_fork");
	bpf_ringbuf_submit(e, 0);
	return 0;
}
