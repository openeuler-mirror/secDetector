/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zcfsite
 * create: 2023-09-21
 * Description: secDetector main entry.
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include "secDetector_manager.h"
#include "secDetector_response.h"
#include "response_unit/secDetector_proc.h"

struct proc_dir_entry *g_root_dir;

static unsigned int log_size = 4096;
module_param(log_size, uint, 0400);
MODULE_PARM_DESC(log_size, "log size");

static int __init secDetector_init(void)
{
	int ret;
	secDetector_init_manager();
	g_root_dir = proc_mkdir_mode("secDetector", 0500, NULL);
	if (g_root_dir == NULL) {
		pr_err("[secDetector] make proc dir failed\n");
		return -EFAULT;
	}

	ret = secDetector_init_log(g_root_dir, log_size);
	if (ret != 0) {
		pr_err("[secDetector] init log failed\n");
		proc_remove(g_root_dir);
		return ret;
	}

	ret = secDetector_response_init();
	if (ret != 0) {
		pr_err("[secDetector] init ringbuf failed\n");
		secDetector_destroy_log();
		proc_remove(g_root_dir);
		return ret;
	}

	pr_debug("[secDetector] init success\n");
	return 0;
}

static void __exit secDetector_exit(void)
{
	secDetector_response_exit();
	secDetector_destroy_log();
	proc_remove(g_root_dir);

	pr_debug("[secDetector] exit success\n");
}

module_init(secDetector_init);
module_exit(secDetector_exit);
MODULE_LICENSE("GPL");
