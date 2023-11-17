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
 * Create: 2023-11-14
 * Description: secDetector process hook
 */
#include "ebpf_types.h"
#include "secDetector_topic.h"
#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct
{
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
    e->type = CREATPROCESS;
    __builtin_memcpy(e->event_name, "sched_process_exec", sizeof("sched_process_exec"));

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(void)
{
    struct ebpf_event *e = NULL;
    u32 exit_code = 0;

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    get_common_info(e);
    e->type = DESTROYPROCESS;
    __builtin_memcpy(e->event_name, "sched_process_exit", sizeof("sched_process_exit"));
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
    e->type = CREATPROCESS;
    __builtin_memcpy(e->event_name, "sched_process_fork", sizeof("sched_process_fork"));
    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("fentry/commit_creds")
int BPF_PROG(handle_commit_creds, const struct cred *new)
{
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    int old_uid = BPF_CORE_READ(task, cred, uid.val);
    int old_gid = BPF_CORE_READ(task, cred, gid.val);
    int new_uid = new->uid.val;
    int new_gid = new->gid.val;
    struct ebpf_event *e = NULL;

    if (old_uid == new_uid && old_gid == new_gid)
        return 0;

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    e->type = SETPROCESSATTR;
    e->process_info.new_uid = new_uid;
    e->process_info.new_gid = new_gid;
    get_common_info(e);
    __builtin_memcpy(e->event_name, "commit_creds", sizeof("commit_creds"));
    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("fentry/security_bprm_check")
int BPF_PROG(handle_bprm_check, struct linux_binprm *bprm)
{
    struct ebpf_event *e = NULL;

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    e->type = CREATPROCESS;
    bpf_core_read_str(e->process_info.bprm_file, MAX_FILENAME_SIZE, bprm->file->f_path.dentry->d_name.name);
    e->process_info.have_bprm = 1;
    get_common_info(e);
    __builtin_memcpy(e->event_name, "security_bprm_check", sizeof("security_bprm_check"));
    bpf_ringbuf_submit(e, 0);
    return 0;
}
