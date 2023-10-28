/*
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef SECDETECTOR_MANAGER_H
#define SECDETECTOR_MANAGER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include "secDetector_module_type.h"

extern struct proc_dir_entry *g_root_dir;
extern void secDetector_init_manager(void);
extern int secDetector_module_register(struct secDetector_module *module);
extern void secDetector_module_unregister(struct secDetector_module *module);

#endif
