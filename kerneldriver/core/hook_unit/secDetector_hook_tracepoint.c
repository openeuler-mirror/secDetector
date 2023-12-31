/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zcfsite
 * create: 2023-09-21
 * Description: tracepoint hook
 */

#include <trace/events/sched.h>
#include <trace/events/signal.h>
#include "secDetector_hook.h"

typedef int (*REGFUNC)(void *, void *);
typedef int (*UNREGFUNC)(void *, void *);
#define tracepoint_register_call(name) ((REGFUNC)register_trace_##name)
#define tracepoint_unregister_call(name) ((UNREGFUNC)unregister_trace_##name)

struct secDetector_tracepoint {
	void *handler;
	REGFUNC register_func;
	UNREGFUNC unregister_func;
};


static struct secDetector_tracepoint secDetector_tracepoint_hook_functions[] = {
	[HOOKEND] = {
		.handler = NULL,
		.register_func = NULL,
		.unregister_func = NULL,
	}
};

int insert_tracepoint_hook(struct secDetector_workflow *workflow)
{
	int ret = 0;
	struct list_head *head = NULL;
	struct secDetector_tracepoint *tp = NULL;

	if (workflow == NULL)
		return -1;

	if (workflow->hook_type < TRACEPOINT_HOOK_START ||
	    workflow->hook_type > TRACEPOINT_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	if (list_empty(head) == 1) {
		tp = &secDetector_tracepoint_hook_functions
			     [workflow->hook_type - TRACEPOINT_HOOK_START];
		if (tp == NULL || tp->register_func == NULL ||
		    tp->handler == NULL)
			return -1;

		ret = tp->register_func(tp->handler, NULL);
		if (ret != 0)
			return ret;
	}

	list_add_rcu(&workflow->list,
		     &secDetector_hook_array[workflow->hook_type]);

	return ret;
}

int delete_tracepoint_hook(struct secDetector_workflow *workflow)
{
	int ret = 0;
	struct secDetector_tracepoint *tp = NULL;

	if (workflow == NULL)
		return -1;

	if (workflow->hook_type < TRACEPOINT_HOOK_START ||
	    workflow->hook_type > TRACEPOINT_HOOK_END)
		return -1;

	list_del_rcu(&workflow->list);
	synchronize_rcu();

	if (list_empty(&secDetector_hook_array[workflow->hook_type]) == 1) {
		tp = &secDetector_tracepoint_hook_functions
			     [workflow->hook_type - TRACEPOINT_HOOK_START];
		if (tp == NULL || tp->register_func == NULL ||
		    tp->handler == NULL)
			return -1;

		ret = tp->unregister_func(tp->handler, NULL);
	}

	return ret;
}

bool tracepoint_exists(struct secDetector_workflow *workflow)
{
	struct secDetector_workflow *tmp_wf = NULL;
	struct list_head *head = NULL;
	if (workflow == NULL)
		return false;

	if (workflow->hook_type < TRACEPOINT_HOOK_START ||
	    workflow->hook_type > TRACEPOINT_HOOK_END)
		return -1;

	head = &secDetector_hook_array[workflow->hook_type];
	list_for_each_entry (tmp_wf, head, list) {
		if (tmp_wf == workflow)
			return true;
	}
	return false;
}
