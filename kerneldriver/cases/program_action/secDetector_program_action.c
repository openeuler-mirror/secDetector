/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: hurricane618
 * create: 2023-11-17
 * Description: program action of kprobe case
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <trace/events/sched.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/signal.h>
#include <linux/pid.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>
#include <linux/printk.h>
#include <linux/kprobes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/utsname.h>
#include <linux/nsproxy.h>
#include <linux/pipe_fs_i.h>
#include <uapi/linux/prctl.h>
#include <asm/ptrace.h>
#include <net/net_namespace.h>
#include <linux/ipc_namespace.h>
#include <linux/user_namespace.h>
#include <linux/cgroup.h>
#include <linux/types.h>
#include <linux/fs_struct.h>
#include <linux/preempt.h>
#include <linux/fsnotify.h>
#include <linux/mm_types.h>
#include <linux/dcache.h>
#include <linux/uaccess.h>
#include <linux/pid_namespace.h>
#include <linux/ctype.h>
#include <linux/cred.h>
#include <linux/kthread.h>
#include <string.h>

#include "secDetector_manager.h"
#include "secDetector_response.h"
#include <secDetector_module_type.h>

#define PATH_LEN 512
#define TIME_STR_MAX_LEN 100
#define BUF_SIZE 4096

DEFINE_MUTEX(program_action_mutex);

struct mnt_namespace {
	atomic_t count;
	struct ns_common ns;
};

struct process_info {
	uid_t uid;
	char exebuf[PATH_LEN];
	char *exe;
	char rootbuf[PATH_LEN];
	const char *root;
	char cwdbuf[PATH_LEN];
	const char *cwd;
	pid_t pid;
	pid_t ppid;
	pid_t pgid;
	pid_t tgid;
	pid_t sid;
	pid_t tracer_pid;
	char comm[TASK_COMM_LEN];
	char pcomm[TASK_COMM_LEN];
	char nodename[255];
	unsigned int pns;
	unsigned int root_pns;
	unsigned int net_ns;
	unsigned int ipc_ns;
	unsigned int mnt_ns;
	unsigned int uts_ns;
	unsigned int user_ns;
	unsigned int cgroup_ns;
	int umask;
};

int get_timestamp_str(char **ret_str)
{
	struct timespec64 ts;
	struct tm stm;
	char *stm_str;
	int stm_str_len = 0;

	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, 0, &stm);

	stm_str = (char *)kzalloc(TIME_STR_MAX_LEN, GFP_ATOMIC);
	if (stm_str == NULL) {
		pr_err("kzalloc failed\n");
		*ret_str = NULL;
		return 0;
	}

	stm_str_len = scnprintf(stm_str, TIME_STR_MAX_LEN,
			"timestamp=%04ld%02d%02d.%02d:%02d:%02d ",
			stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
	if (stm_str_len <= 0) {
		pr_err("scnprintf failed\n");
		kfree(stm_str);
		*ret_str  = NULL;
		return 0;
	}

	*ret_str = kstrdup(stm_str, GFP_KERNEL);
	if (*ret_str == NULL) {
		pr_err("kstrdup failed\n");
		stm_str_len = 0;
	}

	kfree(stm_str);
	return stm_str_len;
}

char *get_process_path(struct task_struct *p, char *pathname, int len)
{
	char *process_path = NULL;
	struct file *exe_file = NULL;

	if (pathname == NULL || p == NULL || len <= 0)
		return NULL;
	
	if (current->flags & PF_EXITING) {
		pr_warn("current task is exiting, we cannot get_process_path\n");
		return NULL;
	}

	exe_file = get_task_exe_file(p);
	if (exe_file != NULL) {
		process_path = d_path(&exe_file->f_path, pathname, len);
		fput(exe_file);
	}

	if (process_path == NULL) {
		strcpy(pathname, p->comm);
		process_path = pathname;
	}
	return process_path;
}

static int get_task_root(struct task_struct *task, struct path *root)
{
	int result = -ENOENT;

	task_lock(task);
	if (task->fs) {
		get_fs_root(task->fs, root);
		result = 0;
	}
	task_unlock(task);
	return result;
}

static int get_task_cwd(struct task_struct *task, struct path *cwd)
{
	int result = -ENOENT;

	task_lock(task);
	if (task->fs) {
		get_fs_pwd(task->fs, cwd);
		result = 0;
	}
	task_unlock(task);
	return result;
}

static void put_common_process_info(struct process_info *pi)
{
	kfree(pi);
}

static struct process_info *get_common_process_info(struct task_struct *tsk, struct task_struct *parent)
{
	struct process_info *pi;
	struct nsproxy *nsproxy;
	struct path root;
	struct path cwd;
	struct task_struct *tracer;
	if (!tsk)
		return NULL;
	
	pi = kzalloc(sizeof(*pi), GFP_ATOMIC);
	if (!pi)
		return NULL;

	tracer = ptrace_parent(tsk);
	if (tracer)
		pi->tracer_pid = task_pid_nr(tracer);

	pi->umask = tsk->fs->umask;
	pi->uid = tsk->cred->uid.val;
	pi->pid = tsk->pid;
	pi->tgid = tsk->pid;
	memcpy(pi->comm, tsk->comm, TASK_COMM_LEN);
	pi->exe = get_process_path(tsk, pi->exebuf, PATH_LEN);
	if (!pi->exe) {
		put_common_process_info(pi);
		return NULL;
	}
	pi->sid = pid_vnr(task_session(tsk));
	pi->pgid = pid_vnr(task_pgrp(tsk));
	pi->pns = task_active_pid_ns(tsk)->ns.inum;
	pi->root_pns = task_active_pid_ns(&init_task)->ns.inum;

	if (get_task_root(tsk, &root) == 0) {
		pi->root = d_path(&root, pi->rootbuf, PATH_LEN);
	}
	if (IS_ERR_OR_NULL(pi->root)) {
		pi->root = "invalid";
	}

	if (get_task_cwd(tsk, &cwd) == 0) {
		pi->cwd = d_path(&cwd, pi->cwdbuf, PATH_LEN);
	}
	if (IS_ERR_OR_NULL(pi->cwd)) {
		pi->cwd = "invalid";
	}

	nsproxy = tsk->nsproxy;
	if (nsproxy) {
		if (nsproxy->net_ns) {
			pi->net_ns = nsproxy->net_ns->ns.inum;
		}

		if (nsproxy->cgroup_ns) {
			pi->cgroup_ns = nsproxy->cgroup_ns->ns.inum;
		}

		if (nsproxy->ipc_ns) {
			pi->ipc_ns = nsproxy->ipc_ns->ns.inum;
		}

		if (nsproxy->uts_ns) {
			pi->uts_ns = nsproxy->uts_ns->ns.inum;
		}

		if (nsproxy->mnt_ns) {
			pi->mnt_ns = nsproxy->mnt_ns->ns.inum;
		}
	}

	pi->user_ns = __task_cred(tsk)->user_ns->ns.inum;

	strncpy(pi->nodename, tsk->nsproxy->uts_ns->name.nodename, 254);

	rcu_read_lock();
	if (!parent) {
		pi->ppid = task_tgid_vnr(rcu_dereference(tsk->real_parent));
		memcpy(pi->pcomm, rcu_dereference(tsk->real_parent)->comm, TASK_COMM_LEN);
	} else {
		pi->ppid = task_tgid_vnr(parent);
		memcpy(pi->pcomm, parent->comm, TASK_COMM_LEN);
	}
	rcu_read_unlock();
	return pi;
}

static int ptrace_attach_pre_handler(struct secDetector_workflow *wf,
				struct kprobe *p, struct pt_regs *regs)
{
	struct task_struct *attach_task = NULL;
	long request = 0;
	unsigned long addr = 0;
	unsigned long flags = 0;
	struct process_info *pi = get_common_process_info(current, NULL);
	char *timestamp = NULL;
	int timestamp_len = 0;
	response_data_t log;

	if (!pi) {
		pr_warn("get_common_process_info by fork failed\n");
		return 0;
	}

#ifdef CONFIG_X86_64
	attach_task = (struct task_struct*)regs->di;
	request = (long)regs->si;
	addr = (unsigned long)regs->dx;
	flags = (unsigned long)regs->r10;
#endif
#ifdef CONFIG_ARM64
	attach_task = (struct task_struct*)regs->regs[0];
	request = (long)regs->regs[1];
	addr = (unsigned long)regs->regs[2];
	flags = (unsigned long)regs->regs[3];
#endif
	if (!attach_task) {
		pr_err("ptrace_attach input task_struct error or arch don't support\n");
		return 0;
	}

	timestamp_len = get_timestamp_str(&timestamp);

	log.report_data.type = 0x00000800;
	log.report_data.len = BUF_SIZE;
	log.report_data.text = kzalloc(BUF_SIZE, GFP_ATOMIC);
	snprintf(log.report_data.text, BUF_SIZE,
			 "%s event_type=call_api uid=%d exe=%s pid=%d comm=%s tgid=%d ppid=%d pcomm=%s pgid=%d sid=%d nodename=%s pns=%u root_pns=%u api_name=%s api_arg=[attach_task_pid=%d cur_task_pid=%d request=%ld addr=%lu flags=%lu]\n",
			 timestamp, pi->uid, pi->exe, pi->pid, pi->comm, pi->tgid, pi->ppid, pi->pcomm, pi->pgid, pi->sid, pi->nodename, pi->pns, pi->root_pns,
			 "ptrace_attach", attach_task->pid, current->pid, request, addr, flags);

	secDetector_report(&log);
	kfree(log.report_data.text);
 	put_common_process_info(pi);

	return 0;
}

static int do_pipe2_pre_handler(struct secDetector_workflow *wf,
				struct kprobe *p, struct pt_regs *regs)
{
	struct process_info *pi = get_common_process_info(current, NULL);
	char *timestamp = NULL;
	int timestamp_len = 0;
	response_data_t log;

	if (!pi) {
		pr_warn("get_common_process_info by fork failed\n");
		return 0;
	}
	timestamp_len = get_timestamp_str(&timestamp);

	log.report_data.type = 0x00000200;
	log.report_data.len = BUF_SIZE;
	log.report_data.text = kzalloc(BUF_SIZE, GFP_ATOMIC);
	snprintf(log.report_data.text, BUF_SIZE,
			 "%s event_type=createpipe uid=%d exe=%s pid=%d comm=%s tgid=%d ppid=%d pcomm=%s pgid=%d sid=%d nodename=%s pns=%u root_pns=%u dfd= pipe_name=%s\n",
			 timestamp, pi->uid, pi->exe, pi->pid, pi->comm, pi->tgid, pi->ppid, pi->pcomm, pi->pgid, pi->sid, pi->nodename, pi->pns, pi->root_pns,
			 "");

	secDetector_report(&log);
	kfree(log.report_data.text);
	put_common_process_info(pi);

	return 0;
}

static struct secDetector_workflow workflow_array[] = {
	{
		.workflow_type = WORKFLOW_CUSTOMIZATION,
		.workflow_func.kprobe_func = ptrace_attach_pre_handler,
		.hook_type = KPROBE_PTRACE_ATTACH,
		.enabled = ATOMIC_INIT(true)
    },
	{
		.workflow_type = WORKFLOW_CUSTOMIZATION,
		.workflow_func.kprobe_func = do_pipe2_pre_handler,
		.hook_type = KPROBE_DO_PIPE2,
		.enabled = ATOMIC_INIT(true)
	},
};

static struct secDetector_module kprobe_program_action = {
	.name = "secDetector kprobe program action module",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
};

static int __init register_secDetector_program_action(void)
{
	int ret;
	ret = secDetector_module_register(&kprobe_program_action);
	if (ret < 0)
		pr_err("[secDetector case program_action] register failed");
	else
		pr_info("[secDetector case program_action] register success\n");

	return ret;
}

static void __exit unregister_secDetector_program_action(void)
{
	mutex_lock(&program_action_mutex);
	(void)secDetector_module_unregister(&kprobe_program_action);
	mutex_unlock(&program_action_mutex);

	pr_info("[secDetector case program_action] unregister success\n");
}

module_init(register_secDetector_program_action);
module_exit(unregister_secDetector_program_action);
MODULE_LICENSE("GPL");

