/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector module type header
 */
#ifndef SECDETECTOR_MODULE_TYPE_H
#define SECDETECTOR_MODULE_TYPE_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include "secDetector_workflow_type.h"

typedef struct secDetector_workflow secDetector_workflow_t;
struct secDetector_module {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int id;
	char *name;
	struct module *kmodule;
	atomic_t enabled;

	secDetector_workflow_t *workflow_array;
	uint32_t workflow_array_len;
};

#endif