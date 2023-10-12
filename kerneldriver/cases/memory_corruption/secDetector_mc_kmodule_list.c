/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-09-28
 * Description: the main implement of the kmodule list corruption.
 */
#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "secDetector_mc_kmodule_list.h"

#define MODULE_LIST_MAXSIZE 0x10000

// 3 ways for get kernel module list.
// struct module->list
// struct module->mkobj->kobj->entry
// struct module->mkobj->kobj->kset

void check_kmodule_list(void)
{
	struct module_kobject *mobj = NULL;
	struct kobject *k = NULL;
	struct module *m = NULL;
	struct kset *module_kset = __this_module.mkobj.kobj.kset;

	if (module_kset == NULL)
	return;

	spin_lock(&module_kset->list_lock);
	list_for_each_entry(k, &module_kset->list, entry) {
		if (k->name == NULL)
			continue;
		mobj = container_of(k, struct module_kobject, kobj);
		if (mobj == NULL || mobj->mod == NULL || (unsigned long)mobj->mod->name < MODULE_LIST_MAXSIZE)
			continue;

		mutex_lock(&module_mutex);
		m = find_module(k->name);
		if (m == NULL)
			pr_err("[secDetector] mc kmoudle list find! module_name=%s.\n", k->name);
		mutex_unlock(&module_mutex);
	}
	spin_unlock(&module_kset->list_lock);
	return;
}