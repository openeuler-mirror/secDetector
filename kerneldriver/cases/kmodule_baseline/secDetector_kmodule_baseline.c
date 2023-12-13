/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-7
 * Description: the main implement of kmodule baseline.
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include "secDetector_manager.h"
#include <secDetector_module_type.h>
#include "secDetector_mc_kmodule_baseline.h"

#define KMOD_BASELINE_TIME_INTERVAL 5
DEFINE_MUTEX(case_kmodule_mutex);

static void check_watching_kmodule(void)
{
	if (mutex_trylock(&case_kmodule_mutex) == 0) {
		pr_warn("[secDetector case kmodule baseline] check cann't getlock, ret\n");
		return;
	}
        check_kmodule_baseline();
        mutex_unlock(&case_kmodule_mutex);
}

static struct secDetector_workflow workflow_array[] = {
	{
		.workflow_type = WORKFLOW_CUSTOMIZATION,
		.workflow_func.func = check_watching_kmodule,
		.hook_type = SECDETECTOR_TIMER,
		.interval = KMOD_BASELINE_TIME_INTERVAL,
		.enabled = ATOMIC_INIT(true)
        },
};

static struct secDetector_module mc_kmodule = {
	.name = "secDetector kmodule baseline module",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
};

static int __init register_secDetector_kmodule_baseline(void)
{
	int ret;
	ret = secDetector_module_register(&mc_kmodule);
	if (ret < 0)
		pr_err("[secDetector case kmodule baseline] register failed");
	else
		pr_info("[secDetector case kmodule baseline] register success\n");

	return ret;
}

static void __exit unregister_secDetector_kmodule_baseline(void)
{
	mutex_lock(&case_kmodule_mutex);
	(void)secDetector_module_unregister(&mc_kmodule);
	mutex_unlock(&case_kmodule_mutex);

	pr_info("[secDetector case kmodule baseline] unregister success\n");
}

module_init(register_secDetector_kmodule_baseline);
module_exit(unregister_secDetector_kmodule_baseline);
MODULE_LICENSE("GPL");

