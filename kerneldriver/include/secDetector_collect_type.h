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
	COLLECT_GLOBAL_FUNCTION_SWITCH,
	COLLECT_GLOBAL_RESOURCE,
	COLLECT_GLOBAL_END = COLLECT_GLOBAL_RESOURCE,
	COLLECT_CUSTOMIZATION,
	NR_COLLECT,
};

enum COLLECT_VALUE_TYPE {
	COLLECT_VALUE_INT,
	COLLECT_VALUE_REGION,
};

struct mem_region {
	void *addr;
	uint32_t size;
};
union collect_value_type {
	unsigned long long int_value;
	struct mem_region region_value;
};

struct collect_data {
	struct list_head list;
	struct rcu_head rcu;
	char *name;
	int value_type;
	union collect_value_type value;
};

typedef void* current_context_t;
typedef void (*collect_func_t)(current_context_t, struct list_head *);

struct secDetector_collect {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int collect_type;
	collect_func_t collect_func;
};

/*可以考虑把 type这个字段作为一个id，用来标识collect unit，
从而避免同一个hook点下的不同的workflow的相同 collect unit重复执行。
但是那样需要增加支持多自定义unit的id的分配，管理，查询工作，也很繁琐，
当前未感知到明确的性能提升,所以并未实施。
*/
#endif