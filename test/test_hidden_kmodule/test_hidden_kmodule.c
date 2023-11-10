/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-10
 * Description: test for hiding kmodule
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm-generic/errno.h>
#include <asm/tlbflush.h>

#include "hide_kmodule.h"
#include "hide_utils.h"

MODULE_LICENSE("GPL");

#define FILE_MASK 0644

static struct proc_dir_entry *proc_root = NULL;
static struct proc_dir_entry *hide_module_option = NULL;

static int g_module_hide_enable = 0;

static ssize_t module_option_write(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	long val = 0;
	char *end = NULL;
	char *tmp = kzalloc((count + 1), GFP_KERNEL);

	if (tmp == NULL)
		return -ENOMEM;

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	
	val = simple_strtol(tmp, &end, 10);
	kfree(tmp);
	if (g_module_hide_enable == 0 && val > 0) {
		hide_self();
		g_module_hide_enable = 1;
	} else if (g_module_hide_enable != 0 && val == 0) {
		unhide_self();
		g_module_hide_enable = 0;
	}

	return count;
}

static struct proc_ops hide_module_fops = {
	.proc_write = module_option_write,
};

static int hide_init(void)
{
	int ret;
	
	ret = init_symbol_lookup_func();
	if (ret != 0) {
		pr_err("hide init symbol lookup func failed\n");
		return -1;
	}

	proc_root = proc_mkdir("secDetector_test_hidden_kmodule", NULL);
	if (proc_root == NULL) {
		pr_err("proc mkdir failed\n");
		return -1;
	}

	hide_module_option = proc_create("hide_kmodule", FILE_MASK, proc_root, &hide_module_fops);
	if (hide_module_option == NULL) {
		pr_err("create hide kmodule failed\n");
		return -1;
	}

	return 0;
}

static int test_hidden_kmodule_init(void)
{
	hide_init();
	return 0;
}

static void test_hidden_kmodule_exit(void)
{
	if (hide_module_option)
		proc_remove(hide_module_option);
	
	if (proc_root)
		proc_remove(proc_root);
}

module_init(test_hidden_kmodule_init)
module_exit(test_hidden_kmodule_exit)


