/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-11-17
 * Description: the collect unit of time func.
 */
#include "secDetector_time.h"
#include "secDetector_collect.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <time.h>

void collect_time(current_context_t cc, struct list_head *ret_list)
{
	struct timespec64 ts;
	struct tm *stm;
	const char *name = "time";
	struct collect_data *cd;

	stm = (struct tm *)kmalloc(sizeof(struct tm), GFP_KERNEL);
	if (stm == NULL) {
		pr_err("kmalloc failed\n");
		return;
	}
	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, 0, stm);

	cd = init_collect_data(name);
	if (cd == NULL) {
		kfree(stm);
		return;
	}
	cd->value_type = COLLECT_VALUE_REGION;
	cd->value.region_value.addr = (void *)stm;
	cd->value.region_value.size = sizeof(struct tm);

	list_add_tail(&cd->list, ret_list);
}

