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
	do_secDetector_hook_callback(kprobe_func, KPROBE_VFS_UNLINK, &ret,
				     PARAMS(p, regs));
	return 0;
}

static int sys_memfd_create_handler(struct kprobe *p, struct pt_regs *regs)
{
	int ret;
	do_secDetector_hook_callback(kprobe_func, KPROBE_MEMFD_CREATE, &ret,
				     PARAMS(p, regs));
	return 0;
}

static struct kprobe secDetector_kprobe_hook_functions[] = {
	[KPROBE_VFS_UNLINK] = {
		.symbol_name = "vfs_unlink",
		.pre_handler = vfs_unlink_handler,
	},
	[KPROBE_MEMFD_CREATE] = {
		.symbol_name = "sys_memfd_create",
		.pre_handler = sys_memfd_create_handler,
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

		unregister_kprobe(kp);
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
