/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-08
 * Description: the collect unit func.
 */
#include "secDetector_collect.h"
#include "secDetector_function_switch.h"
#include "secDetector_time.h"
#include <linux/slab.h>

collect_func_t collect_units[NR_COLLECT] = {
	[COLLECT_GLOBAL_FUNCTION_SWITCH] = collect_function_switch,
	[COLLECT_TIME] = collect_time,
};


struct collect_data *init_collect_data(const char *name)
{
	int nl;
	struct collect_data *cd;
	if (name == NULL)
		return NULL;
	cd = kmalloc(sizeof(struct collect_data), GFP_KERNEL);
	if (cd == NULL) {
		pr_err("kmalloc failed");
		return NULL;
	}

	nl = strlen(name);
	cd->name = kmalloc(nl + 1, GFP_KERNEL);
	if (cd->name == NULL) {
		pr_err("kmalloc failed");
		kfree(cd);
		return NULL;
	}
	strncpy(cd->name, name, nl);
	cd->name[nl] = '\0';

	return cd;
}

void free_collect_data(struct collect_data *cd)
{
	kfree(cd->name);
	if (cd->value_type == COLLECT_VALUE_REGION)
		kfree(cd->value.region_value.addr);
	kfree(cd);
}