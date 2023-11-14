/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-11-13
 * Description: lsm hook
 */
#include <linux/lsm_hooks.h>
#include <linux/kprobes.h>
#include "secDetector_hook.h"

char *lsm_name = "secDetector";
struct security_hook_heads *security_hook_heads_p;

struct secDetector_lsm_hooklist {
	struct security_hook_list lsm_hooklist;
	size_t offset;
};

#define SECDETECTOR_LSM_HOOK_INIT_HOOK(HEAD, HOOK) \
		{.offset = offsetof(struct security_hook_heads, HEAD),   \
		.lsm_hooklist.hook = {.HEAD = HOOK}}

static int inode_mkdir_handler(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int ret = 0;
	do_secDetector_hook_callback_valid_ret(lsm_inode_mkdir_func, LSM_INODE_MKDIR, &ret,
				     PARAMS(dir, dentry, mode));
					 return ret;
}

static struct secDetector_lsm_hooklist secDetector_lsm_hooklists[] = {
	[LSM_INODE_MKDIR] = SECDETECTOR_LSM_HOOK_INIT_HOOK(inode_mkdir, inode_mkdir_handler),
};


static int find_kallsyms_lookup_name_func(void **kallsyms_lookup_name_func_addr)
{
	struct kprobe kp = {.symbol_name = "kallsyms_lookup_name",};
	int ret = 0;
	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("[secDetector] register_kprobe failed error: %d\n", ret);
		return ret;
	}

	*kallsyms_lookup_name_func_addr = kp.addr;
	unregister_kprobe(&kp);
	return ret;
}

static void secDetector_security_add_hooks(struct secDetector_lsm_hooklist *slh, int count, char *lsm)
{
	int i;
	for (i = 0; i < count; i++) {
		slh[i].lsm_hooklist.head = (struct hlist_head *)((void *)security_hook_heads_p + slh[i].offset);
		slh[i].lsm_hooklist.lsm = lsm;
		hlist_add_tail_rcu(&slh[i].lsm_hooklist.list, slh[i].lsm_hooklist.head);
	}
}

int init_lsm_hook(void)
{
	int ret;
	unsigned long (*kallsyms_lookup_name_func)(const char *name) = NULL;
	void *kallsyms_lookup_name_func_add = NULL;
	ret = find_kallsyms_lookup_name_func(&kallsyms_lookup_name_func_add);
	if (ret < 0) {
		return ret;
	}
	kallsyms_lookup_name_func = (long unsigned int (*)(const char *))kallsyms_lookup_name_func_add;

	security_hook_heads_p = (struct security_hook_heads *) kallsyms_lookup_name_func("security_hook_heads");
	if (!security_hook_heads_p) {
		pr_err("[secDetector] main find security_hook_heads failed\n");
		return -1;
	}

	return 0;
}


int insert_lsm_hook(struct secDetector_workflow *workflow)
{
	int ret = 0;
	struct list_head *head = NULL;
	struct secDetector_lsm_hooklist *shl = NULL;

	if (workflow == NULL)
		return -1;
	if (workflow->hook_type < LSM_HOOK_START ||
	    workflow->hook_type > LSM_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	if (list_empty(head) == 1) {
		shl = &secDetector_lsm_hooklists
			     [workflow->hook_type - LSM_HOOK_START];
		if (shl == NULL)
			return -1;

		secDetector_security_add_hooks(shl, 1, lsm_name);
	}

	list_add_rcu(&workflow->list,
		     &secDetector_hook_array[workflow->hook_type]);

	return ret;
}

int delete_lsm_hook(struct secDetector_workflow *workflow)
{
	int ret = 0;
	struct secDetector_lsm_hooklist *shl = NULL;

	if (workflow == NULL)
		return -1;

	if (workflow->hook_type < LSM_HOOK_START ||
	    workflow->hook_type > LSM_HOOK_END)
		return -1;

	list_del_rcu(&workflow->list);
	synchronize_rcu();

	if (list_empty(&secDetector_hook_array[workflow->hook_type]) == 1) {
		shl = &secDetector_lsm_hooklists
			     [workflow->hook_type - LSM_HOOK_START];
		if (shl == NULL)
			return -1;

		security_delete_hooks(&shl->lsm_hooklist, 1);
	}

	return ret;
}

bool lsm_exists(struct secDetector_workflow *workflow)
{
	struct secDetector_workflow *tmp_wf = NULL;
	struct list_head *head = NULL;
	if (workflow == NULL)
		return false;

	if (workflow->hook_type < LSM_HOOK_START ||
	    workflow->hook_type > LSM_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	list_for_each_entry (tmp_wf, head, list) {
		if (tmp_wf == workflow)
			return true;
	}
	return false;
}
