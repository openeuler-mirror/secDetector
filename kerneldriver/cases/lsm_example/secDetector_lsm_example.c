/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-11-14
 * Description: expample of lsm hook unit case
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "secDetector_manager.h"
#include <secDetector_module_type.h>

#include <linux/dcache.h>
#include <linux/path.h>

DEFINE_MUTEX(case_lsm_mutex);

static int inode_mkdir_handler(struct secDetector_workflow *wf, struct inode *dir, struct dentry *dentry, umode_t mode)
{
	char *buffer = NULL;
	char *pathname = NULL;

	buffer = kmalloc(PATH_MAX, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err("[secDetector] test_inode_mkdir: kmalloc failed.\n");
		return 0;

	}
	pathname = dentry_path_raw(dentry, buffer, PATH_MAX);
	if (!IS_ERR(pathname))
		pr_info("[secDetector] test_inode_mkdir: path: %s!!! \n", pathname);
	kfree(buffer);
	return 0;
}

static struct secDetector_workflow workflow_array[] = {
	{
		.workflow_type = WORKFLOW_CUSTOMIZATION,
		.workflow_func.lsm_inode_mkdir_func = inode_mkdir_handler,
		.hook_type = LSM_INODE_MKDIR,
		.enabled = ATOMIC_INIT(true)
        },
};

static struct secDetector_module lsm_example = {
	.name = "secDetector lsm example module",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
};

static int __init register_secDetector_lsm_example(void)
{
	int ret;
	mutex_lock(&case_lsm_mutex);
	ret = secDetector_module_register(&lsm_example);
	if (ret < 0)
		pr_err("[secDetector case lsm_example] register failed");
	else
		pr_info("[secDetector case lsm_example] register success\n");
	mutex_unlock(&case_lsm_mutex);
	return ret;
}

static void __exit unregister_secDetector_lsm_example(void)
{
	mutex_lock(&case_lsm_mutex);
	(void)secDetector_module_unregister(&lsm_example);
	mutex_unlock(&case_lsm_mutex);

	pr_info("[secDetector case lsm_example] unregister success\n");
}

module_init(register_secDetector_lsm_example);
module_exit(unregister_secDetector_lsm_example);
MODULE_LICENSE("GPL");

