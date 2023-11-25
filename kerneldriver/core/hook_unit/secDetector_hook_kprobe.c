/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zcfsite
 * create: 2023-09-21
 * Description: kprobe hook
 */

#include <linux/kprobes.h>
#include "secDetector_hook.h"

static int vfs_unlink_handler(struct kprobe *p, struct pt_regs *regs)
{
	int ret;
	do_secDetector_hook_callback_valid_ret(kprobe_func, KPROBE_VFS_UNLINK, &ret,
				     PARAMS(p, regs));
	return 0;
}

static int ptrace_attach_handler(struct kprobe *p, struct pt_regs *regs)
{
	int ret;
	do_secDetector_hook_callback_valid_ret(kprobe_func, KPROBE_PTRACE_ATTACH, &ret,
				     PARAMS(p, regs));
	return 0;
}

static int do_pipe2_handler(struct kprobe *p, struct pt_regs *regs)
{
	int ret;
	do_secDetector_hook_callback_valid_ret(kprobe_func, KPROBE_DO_PIPE2, &ret,
				     PARAMS(p, regs));
	return 0;
}

static struct kprobe secDetector_kprobe_hook_functions[] = {
	[KPROBE_PTRACE_ATTACH] = {
		.symbol_name = "ptrace_attach",
		.pre_handler = ptrace_attach_handler,
	},
	[KPROBE_DO_PIPE2] = {
		.symbol_name = "do_pipe2",
		.pre_handler = do_pipe2_handler,
	},
};

int insert_kprobe_hook(struct secDetector_workflow *workflow)
{
	int ret = 0;
	struct list_head *head = NULL;
	struct kprobe *kp = NULL;

	if (workflow == NULL)
		return -1;

	if (workflow->hook_type < KPROBE_HOOK_START ||
	    workflow->hook_type > KPROBE_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	if (list_empty(head)) {
		kp = &secDetector_kprobe_hook_functions
			     [workflow->hook_type - KPROBE_HOOK_START];
		if (!kp)
			return -1;
		ret = register_kprobe(kp);
		if (ret < 0)
			return ret;
	}

	list_add_rcu(&workflow->list,
		     &secDetector_hook_array[workflow->hook_type]);

	return ret;
}

int delete_kprobe_hook(struct secDetector_workflow *workflow)
{
	struct kprobe *kp = NULL;
	const char *tmp_sym = NULL;
	kprobe_pre_handler_t tmp_handler;

	if (workflow == NULL)
		return -1;

	if (workflow->hook_type < KPROBE_HOOK_START ||
	    workflow->hook_type > KPROBE_HOOK_END)
		return -1;

	list_del_rcu(&workflow->list);
	synchronize_rcu();

	if (list_empty(&secDetector_hook_array[workflow->hook_type])) {
		kp = &secDetector_kprobe_hook_functions
			     [workflow->hook_type - KPROBE_HOOK_START];
		if (!kp)
			return -1;

		tmp_sym = kp->symbol_name;
		tmp_handler = kp->pre_handler;

		unregister_kprobe(kp);
		//register mutiple times
		memset(kp, 0, sizeof(struct kprobe));
		kp->symbol_name = tmp_sym;
		kp->pre_handler = tmp_handler;
	}

	return 0;
}

bool kprobe_exists(struct secDetector_workflow *workflow)
{
	struct secDetector_workflow *tmp_wf = NULL;
	struct list_head *head = NULL;
	if (workflow == NULL)
		return false;

	if (workflow->hook_type < KPROBE_HOOK_START ||
	    workflow->hook_type > KPROBE_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	list_for_each_entry (tmp_wf, head, list) {
		if (tmp_wf == workflow)
			return true;
	}
	return false;
}
