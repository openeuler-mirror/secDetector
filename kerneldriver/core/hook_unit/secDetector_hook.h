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
 *
 * Author: zcfsite
 * create: 2023-09-21
 * Description: hook head file
 */

#ifndef SECDETECTOR_HOOK_H
#define SECDETECTOR_HOOK_H

#include <linux/mutex.h>
#include <linux/tracepoint.h>
#include "secDetector_workflow_type.h"

extern struct list_head secDetector_hook_array[HOOKEND];
extern struct mutex g_hook_list_array_mutex;

extern int insert_tracepoint_hook(struct secDetector_workflow *workflow);
extern int delete_tracepoint_hook(struct secDetector_workflow *workflow);
extern bool tracepoint_exists(struct secDetector_workflow *workflow);

struct secDetector_timer {
	struct list_head list;
	struct rcu_head rcu;
	int interval;
	struct timer_list timer;
	struct list_head callback_list;
	struct work_struct work;
};

extern int insert_callback(struct secDetector_workflow *workflow);
extern int delete_callback(struct secDetector_workflow *workflow);
extern void init_secDetector_hook(void);

// for timer
#define _do_secDetector_callback(func, callback_list,args)     \
	do {                                                   \
		struct secDetector_workflow *workflow;    \
		if (mutex_trylock(&g_hook_list_array_mutex) == 0)    \
			return;                                 \
		list_for_each_entry_rcu(workflow, &(callback_list), list) {     \
			if (atomic_read(&workflow->enabled) && atomic_read(&workflow->module->enabled))   \
				workflow->workflow_func.func(PARAMS(args));    \
		}      \
		mutex_unlock(&g_hook_list_array_mutex);     \
	} while(0)

//for tracepoint
#define _do_secDetector_callback_atomic(func, callback_list,args)     \
        do {                                                   \
                struct secDetector_workflow *workflow;    \
                rcu_read_lock();    \
                list_for_each_entry_rcu(workflow, &(callback_list), list) {     \
                        if (atomic_read(&workflow->enabled) && atomic_read(&workflow->module->enabled))   \
                                workflow->workflow_func.func(PARAMS(args));    \
                }      \
                rcu_read_unlock();     \
        } while(0)

#define do_secDetector_hook_callback(func, id, args)     \
	_do_secDetector_callback_atomic(func, secDetector_hook_array[id], PARAMS(args))

#endif
