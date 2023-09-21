/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2. 
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2 
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, 
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, 
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.  
 * See the Mulan PSL v2 for more details.
 */


#ifndef SECDETECTOR_MANAGER_H
#define SECDETECTOR_MANAGER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>

enum CALLBACKTYPE {
	TRACEPOINT_HOOK_START,
	TRACEPOINT_CREATE_FILE = TRACEPOINT_HOOK_START,
	TRACEPOINT_WRITE_FILE,
	TRACEPOINT_CREATE_PROCESS,
	TRACEPOINT_HOOK_END = TRACEPOINT_CREATE_PROCESS,

	HOOKEND,

	SECDETECTOR_TIMER,
};

union callback_func {
	void (*func)(void);
	void (*create_file)(struct filename *);
	void (*write_file)(struct filename *);
	void (*create_process)(int);
	void (*timer_func)(struct timer_list *);
};

struct secDetector_workflow {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int id;
	struct seDetector_module *module;
	atomic_t enabled;

	unsigned int hook_type;
	int interval;
	union callback_func callback_func;
};

struct secDetector_module {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int id;
	char *name;
	struct module *kmodule;
	atomic_t enabled;

	struct secDetector_workflow *workflow_array;
	uint32_t workflow_array_len;
};

extern void secDetector_init_manager(void);
extern int secDetector_module_register(struct secDetector_module *module);
extern int secDetector_module_unregister(struct secDetector_module *module);

#endif
