/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector collect unit type header
 */
#ifndef SECDETECTOR_COLLECT_TYPE_H
#define SECDETECTOR_COLLECT_TYPE_H
#include <linux/list.h>
enum COLLECT_TYPE {
	COLLECT_TIME,
	COLLECT_CURRENT_START,
	COLLECT_CURRENT_PROCESS = COLLECT_CURRENT_START,
	COLLECT_CURRENT_FILE,
	COLLECT_CURRENT_END = COLLECT_CURRENT_FILE,
	COLLECT_GLOBAL_START,
	COLLECT_GLOBAL_PROCESS = COLLECT_GLOBAL_START,
	COLLECT_GLOBAL_FILE,
	COLLECT_GLOBAL_RESOURCE,
	COLLECT_GLOBAL_END = COLLECT_GLOBAL_RESOURCE,
	COLLECT_CUSTOMIZATION,
};

union collect_func {
	void (*func)(void);
	void (*COLLECT_record_func)(void);
};

struct secDetector_collect {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int collect_type;
	union collect_func collect_func;
};

#endif