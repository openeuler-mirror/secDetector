/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector workflow type header
 */
#ifndef SECDETECTOR_WORKFLOW_TYPE_H
#define SECDETECTOR_WORKFLOW_TYPE_H

#include <linux/secdetector.h>
#include "secDetector_hook_type.h"
#include "secDetector_collect_type.h"
#include "secDetector_analyze_type.h"
#include "secDetector_response_type.h"
#include "secDetector_module_type.h"

enum WORKFLOW_TYPE {
	WORKFLOW_CUSTOMIZATION,
	WORKFLOW_PRESET,
};
struct secDetector_workflow;
union workflow_func {
	void (*func)(void);
	int (*file_event)(struct secDetector_workflow *,
			  struct secdetector_file *, int);
	int (*task_event)(struct secDetector_workflow *,
			  struct secdetector_task *, int flag);
	void (*timer_func)(struct secDetector_workflow *, struct timer_list *);
	void (*func_wf)(struct secDetector_workflow *);
	int (*kprobe_func)(struct secDetector_workflow *,
				struct kprobe *, struct pt_regs *);
};

typedef struct secDetector_module secDetector_module_t;
typedef struct secDetector_workflow {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int id;
	secDetector_module_t *module;
	atomic_t enabled;
	unsigned int workflow_type;
	union workflow_func workflow_func;

	//hook
	unsigned int hook_type;
	int interval;

	//collect
	struct secDetector_collect *collect_array;
	uint32_t collect_array_len;

	//analyze
	unsigned int analyze_type;
	analyze_func_t analyze_func;

	//response
	struct secDetector_response *response_array;
	uint32_t response_array_len;

	//intermediate status
	analyze_status_t analyze_status;
} secDetector_workflow_t;

#endif
