/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-09-28
 * Description: the main implement of memory corruption probe.
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include "secDetector_manager.h"
#include <secDetector_module_type.h>


#define TIME_INTERVAL 10
DEFINE_MUTEX(case_mc_mutex);
#define KERNELKEYDATATAMPER 0x00008000

static struct secDetector_collect collect_array[] = {
	{
		.collect_type = COLLECT_GLOBAL_FUNCTION_SWITCH,
	},
};

static struct secDetector_workflow workflow_array[] = {
	{
		.workflow_type = WORKFLOW_PRESET,
		.hook_type = SECDETECTOR_TIMER,
		.collect_array = collect_array,
		.analyze_type = ANALYZE_PRESET_SAVE_CHECK,
		.interval = TIME_INTERVAL,
		.enabled = ATOMIC_INIT(true)
	},
};

static struct secDetector_module mc_module = {
	.name = "memory_corruption",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
	.event_type = KERNELKEYDATATAMPER,
};

static int __init register_secDetector_mc(void)
{
	int ret;
	ret = secDetector_module_register(&mc_module);
	if (ret < 0)
		pr_err("[secDetector case memory corruption] register failed");
	else
		pr_info("[secDetector case memory corruption] register success\n");

	return ret;
}

static void __exit unregister_secDetector_mc(void)
{
	mutex_lock(&case_mc_mutex);
	(void)secDetector_module_unregister(&mc_module);
	mutex_unlock(&case_mc_mutex);

	pr_info("[secDetector case memory corruption] unregister success\n");
}

module_init(register_secDetector_mc);
module_exit(unregister_secDetector_mc);
MODULE_LICENSE("GPL");

