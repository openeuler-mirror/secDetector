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

static unsigned int log_size = 4096;
module_param(log_size, uint, 0400);
MODULE_PARM_DESC(log_size, "log size");

#define MIN_RINGBUF_SIZE 4
#define MAX_RINGBUF_SIZE 1024

static unsigned int ringbuf_size = MIN_RINGBUF_SIZE; /* unit is Mb */
static unsigned int ringbuf_size_bytes; /* unit is bytes */
module_param(ringbuf_size, uint, 0400);
MODULE_PARM_DESC(log_size, "ringbuffer size");

static bool ringbuf_size_check(void)
{
	if (ringbuf_size < MIN_RINGBUF_SIZE || ringbuf_size > MAX_RINGBUF_SIZE) {
		pr_err("[secDetector] ringbuf_size should be 4 and 1024 (Mb)\n");
		return false;
	}

	if (!is_power_of_2(ringbuf_size)) {
		pr_err("[secDetector] ringbuf_size should be power of 2\n");
		return false;
	}

	ringbuf_size_bytes = ringbuf_size * 1024 * 1024;
	return true;
}

static int __init secDetector_init(void)
{
	int ret;

	if (!ringbuf_size_check())
		return -EINVAL;

	ret = secDetector_init_manager();
	if (ret != 0) {
		pr_err("[secDetector] init manager failed\n");
		return ret;
	}

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

	ret = secDetector_response_init(ringbuf_size_bytes);
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
