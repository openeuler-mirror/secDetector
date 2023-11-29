/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-3
 * Description: the main implement of the kmodule baseline corruption.
 */

#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <time.h>
#include "secDetector_mc_kmodule_baseline.h"
#include "secDetector_response.h"
#include "secDetector_analyze.h"

#define MODULE_LIST_MAXSIZE 0x10000
#define NAME_LEN 4096
#define KMODULE_BASELINE_TYPE 0x00800000

typedef struct chkrkatt_module {
	struct list_head list;
	char *module_name;
} CHKRKATT_MODULE;

static LIST_HEAD(chkrkatt_module_list);

static int add_kmodule_baseline_name(const char *name)
{
	CHKRKATT_MODULE *module = NULL;
	size_t name_len;

	module = (CHKRKATT_MODULE *)kmalloc(sizeof(CHKRKATT_MODULE), GFP_KERNEL);
	if (!module) {
		pr_err("module kmalloc failed\n");
		return -1;
	}
	module->module_name = (char *)kzalloc(NAME_LEN, GFP_KERNEL);
	if (!module->module_name) {
		pr_err("module_name kzalloc failed\n");
		kfree(module);
		return -1;
	}

	name_len = strlen(name) < NAME_LEN ? strlen(name) : NAME_LEN;		
	memcpy(module->module_name, name, name_len);
	list_add(&module->list, &chkrkatt_module_list);
	return 0;
}

static void free_kmodule_baseline(void)
{
	CHKRKATT_MODULE *module_name = NULL;
	CHKRKATT_MODULE *module_name_next = NULL;
	list_for_each_entry_safe(module_name, module_name_next, &chkrkatt_module_list, list) {
		if (module_name != NULL) {
			list_del(&module_name->list);
			kfree(module_name->module_name);
			kfree(module_name);
		}
	}
}

static void report_kmodule_baseline(void)
{
	CHKRKATT_MODULE *get_module_name = NULL;
	CHKRKATT_MODULE *get_module_name_next = NULL;
	char *module_name_all = NULL;
	char *header_msg = NULL;
	char strtmp[] = ", ";
	int header_msg_len;
	int ret;
	response_data_t log;

	module_name_all = (char *)kzalloc(NAME_LEN, GFP_ATOMIC);
	if (module_name_all == NULL) {
		pr_err("module_name_all kzalloc failed\n");
		return;
	}

	header_msg_len = get_timestamp_str(&header_msg);
	if (header_msg_len <= 0)
		goto out;
	strcat(module_name_all, "event_type=kmodulelist module_name=");
	list_for_each_entry_safe(get_module_name, get_module_name_next, &chkrkatt_module_list, list) {
		if (get_module_name != NULL && get_module_name_next != NULL) {
			/* 2: ', ' */
			if(sizeof(module_name_all) + sizeof(get_module_name->module_name) < NAME_LEN - 2 - header_msg_len) {
				strcat(module_name_all, get_module_name->module_name);
				strcat(module_name_all, strtmp);
			}
		}
	}
	
	log.report_data.type = KMODULE_BASELINE_TYPE;
	log.report_data.len = strlen(module_name_all) + header_msg_len + 1;
	log.report_data.text = kmalloc(log.report_data.len, GFP_KERNEL);

	if (log.report_data.text == NULL) {
		pr_err("log text malloc failed\n");
		goto out;
	}

	ret = scnprintf(log.report_data.text, log.report_data.len, "%s%s\n", header_msg, module_name_all);
	if (ret > 0) {
		log.report_data.text[log.report_data.len - 1] = '\0';
		secDetector_report(&log);
	}
	kfree(log.report_data.text);
out:
	kfree(header_msg);
	kfree(module_name_all);
}

void check_kmodule_baseline(void)
{
	struct module_kobject *mobj = NULL;
	struct kobject *k = NULL;
	struct kset *module_kset = __this_module.mkobj.kobj.kset;
	int ret;

	if (module_kset == NULL)
		return;

	mutex_lock(&module_mutex);
	spin_lock(&module_kset->list_lock);
	list_for_each_entry(k, &module_kset->list, entry) {
		if (k->name == NULL)
			continue;
		mobj = container_of(k, struct module_kobject, kobj);
		if (mobj == NULL || mobj->mod == NULL || (unsigned long)mobj->mod->name < MODULE_LIST_MAXSIZE)
			continue;
		
		ret = add_kmodule_baseline_name(k->name);
		if (ret != 0)
			break;
	}
	spin_unlock(&module_kset->list_lock);
	mutex_unlock(&module_mutex);

	report_kmodule_baseline();
	free_kmodule_baseline();
	return;
}
