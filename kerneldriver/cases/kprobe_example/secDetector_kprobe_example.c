/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zcfsite
 * create: 2023-11-11
 * Description: expample of kprobe case
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kprobes.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "secDetector_manager.h"
#include <secDetector_module_type.h>

DEFINE_MUTEX(case_kprobe_mutex);

static int vfs_unlink_pre_handler(struct secDetector_workflow *wf,
				struct kprobe *p, struct pt_regs *regs)
{
	struct dentry *dent = NULL;
	char *pathname = NULL;
	char *file_path = NULL;

	dent = (struct dentry*)regs->si;
	if (!dent) {
		pr_err("vfs_unlink input dentry error\n");
		return 0;
	}

	pathname = kzalloc(128, GFP_KERNEL);
	if (!pathname)
		return 0;

	file_path = dentry_path_raw(dent, pathname, 128);
	if (!file_path)
		return 0;
	pr_info("data_type=unlinkfile filep_path=%s\n", file_path);

	return 0;
}

static struct secDetector_workflow workflow_array[] = {
	{
		.workflow_type = WORKFLOW_CUSTOMIZATION,
		.workflow_func.kprobe_func = vfs_unlink_pre_handler,
		.hook_type = KPROBE_VFS_UNLINK,
		.enabled = ATOMIC_INIT(true)
        },
};

static struct secDetector_module kprobe_example = {
	.name = "secDetector kprobe example module",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
};

static int __init register_secDetector_kprobe_example(void)
{
	int ret;
	ret = secDetector_module_register(&kprobe_example);
	if (ret < 0)
		pr_err("[secDetector case kprobe_example] register failed");
	else
		pr_info("[secDetector case kprobe_example] register success\n");

	return ret;
}

static void __exit unregister_secDetector_kprobe_example(void)
{
	mutex_lock(&case_kprobe_mutex);
	(void)secDetector_module_unregister(&kprobe_example);
	mutex_unlock(&case_kprobe_mutex);

	pr_info("[secDetector case kprobe_example] unregister success\n");
}

module_init(register_secDetector_kprobe_example);
module_exit(unregister_secDetector_kprobe_example);
MODULE_LICENSE("GPL");

