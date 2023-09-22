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
 * Description: detector hook manager
 */

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include "secDetector_hook.h"

struct hook_list_func {
	int type_min;
	int type_max;
	int (*insert) (struct secDetector_workflow *);
	int (*delete) (struct secDetector_workflow *);
	bool (*exists) (struct secDetector_workflow *);
};

static int insert_timer_callback(struct secDetector_workflow *workflow);
static int unlink_timer_callback(struct secDetector_workflow *workflow);
static int timer_callback_exists(struct secDetector_workflow *workflow);

static struct hook_list_func hook_list_funcs[] = {
	{
		TRACEPOINT_HOOK_START,
		TRACEPOINT_HOOK_END,
		insert_tracepoint_hook,
		delete_tracepoint_hook,
		tracepoint_exists
	},
	{
		SECDETECTOR_TIMER,
		SECDETECTOR_TIMER,
		insert_timer_callback,
		unlink_timer_callback,
		timer_callback_exists
	}
};

struct list_head secDetector_hook_array[HOOKEND];
LIST_HEAD(secDetector_timer_list);
DEFINE_MUTEX(g_hook_list_array_mutex);

void init_secDetector_hook(void)
{
	int i;
	mutex_lock(&g_hook_list_array_mutex);
	for (i = 0; i < HOOKEND; i++)
		INIT_LIST_HEAD(&secDetector_hook_array[i]);

	mutex_unlock(&g_hook_list_array_mutex);
}

static void do_timer_work(struct work_struct *work)
{
	struct secDetector_timer *timer = NULL;

	if (work == NULL)
		return;

	timer = container_of(work, struct secDetector_timer, work);
	_do_secDetector_callback(timer_func, timer->callback_list, PARAMS(&timer->timer));

}

static void add_secDetector_timer(struct secDetector_timer *tm);
static void secDetector_timer_callback(struct timer_list *timer)
{
	struct secDetector_timer *tm = container_of(timer, struct secDetector_timer, timer);
	queue_work(system_unbound_wq, &tm->work);
	add_secDetector_timer(tm);
}

static void add_secDetector_timer(struct secDetector_timer *tm)
{
	tm->timer.expires = jiffies + tm->interval * HZ;
	timer_setup(&tm->timer, secDetector_timer_callback, 0);
	add_timer(&tm->timer);
}

static bool timer_callback_exists(struct secDetector_workflow *workflow)
{
	struct secDetector_timer *timer = NULL;
	struct secDetector_workflow *tmp_wf = NULL;

	if (workflow == NULL)
		return false;

	list_for_each_entry(timer, &secDetector_timer_list, list) {
		if (workflow->interval != timer->interval)
			continue;
		list_for_each_entry(tmp_wf, &timer->callback_list, list) {
			if (tmp_wf == workflow)
				return true;
		}
		return false;
	}
	return false;
}

static int insert_timer_callback(struct secDetector_workflow *workflow)
{
	struct secDetector_timer *timer = NULL;
        if (workflow == NULL)
                return -1;
	
        list_for_each_entry(timer, &secDetector_timer_list, list) {
                if (workflow->interval != timer->interval) {
			list_add_rcu(&workflow->list, &timer->callback_list);
			return 0;
		}
	}

	timer = kzalloc(sizeof(struct secDetector_timer), GFP_KERNEL);
	if (timer == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&timer->list);
	INIT_LIST_HEAD(&timer->callback_list);

	timer->interval = workflow->interval;

	list_add_rcu(&timer->list, &secDetector_timer_list);
	list_add_rcu(&workflow->list, &(timer->callback_list));
	INIT_WORK(&timer->work, do_timer_work);
	add_secDetector_timer(timer);
	return 0;
}

static int unlink_timer_callback(struct secDetector_workflow *workflow)
{
        struct secDetector_timer *timer = NULL;
        if (workflow == NULL)
                return -1;

        list_for_each_entry(timer, &secDetector_timer_list, list) {
                if (workflow->interval != timer->interval) {
			list_del_rcu(&workflow->list);
			synchronize_rcu();
			if (list_empty(&timer->callback_list) == 1) {
				del_timer_sync(&timer->timer);
				list_del_rcu(&timer->list);
				cancel_work_sync(&timer->work);
				kfree_rcu(timer, rcu);
			}
			break;
		}
	}
	return 0;
}

int insert_callback(struct secDetector_workflow *workflow)
{
	int i;
	int ret = -EFAULT;
	struct hook_list_func *list_func = NULL;
        if (workflow == NULL)
                return ret;
	
	for (i = 0; i < ARRAY_SIZE(hook_list_funcs); i++) {
		list_func = &hook_list_funcs[i];
		if (workflow->hook_type >= list_func->type_min &&
			workflow->hook_type <= list_func->type_max) {
			if (list_func->exists(workflow))
				return -EEXIST;
			ret = list_func->insert(workflow);
			break;
		}
	}

	return ret;
}

int delete_callback(struct secDetector_workflow *workflow)
{
        int i;
        int ret = -EFAULT;
        struct hook_list_func *list_func = NULL;
        if (workflow == NULL)
                return ret;

        for (i = 0; i < ARRAY_SIZE(hook_list_funcs); i++) {
                list_func = &hook_list_funcs[i];
                if (workflow->hook_type >= list_func->type_min &&
                        workflow->hook_type <= list_func->type_max) {
                        if (!list_func->exists(workflow))
                                return 0;
                        ret = list_func->delete(workflow);
                        break;
                }
        }

        return ret;
}

