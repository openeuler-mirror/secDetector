/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zcfsite
 * create: 2023-09-21
 * Description: detector module manager
 */

#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "hook_unit/secDetector_hook.h"
#include "secDetector_manager.h"
#include "secDetector_workflow.h"

static DEFINE_IDR(g_module_idr);
static LIST_HEAD(secDetector_module_list);

void secDetector_module_unregister(struct secDetector_module *module)
{
	struct secDetector_workflow *wf = NULL;
	int ret;
	int i;
	void *ret_id = NULL;

	if (module == NULL) {
		pr_err("[secDetector] unregister module is null\n");
		return;
	}

	mutex_lock(&g_hook_list_array_mutex);
	ret_id = idr_remove(&g_module_idr, (unsigned long)module->id);
	if (ret_id == NULL) {
		goto error;
	}

	for (i = 0, wf = module->workflow_array; i < module->workflow_array_len;
		 i++, wf++) {
		if (wf == NULL) {
			goto error;
		}
		ret = delete_callback(wf);
		if (ret < 0) {
			pr_err("[secDetector] delete callback failed\n");
			goto error;
		}
		// workflow在被卸载的时候，需要释放analyze status等申请的内存,特别是使用默认的response list。
		free_analyze_status_data(&wf->analyze_status);
		if (wf->response_array_len == 0) {
			kfree(wf->response_array);
		}
	}

error:
	list_del_rcu(&module->list);
	synchronize_rcu();
	mutex_unlock(&g_hook_list_array_mutex);

	return;
}
EXPORT_SYMBOL_GPL(secDetector_module_unregister);

static void secDetector_collect_analyze_response_unit_padding(struct secDetector_workflow *wf)
{
	int i;
	struct secDetector_collect *sc = wf->collect_array;
	struct secDetector_response *sr = wf->response_array;
	
	for (i = 0; i < wf->collect_array_len; i++) {
		if (sc[i].collect_type < COLLECT_CUSTOMIZATION)
			sc[i].collect_func = collect_units[sc[i].collect_type];
	}

	if (wf->analyze_type < ANALYZE_CUSTOMIZATION) {
		wf->analyze_func = analyze_units[wf->analyze_type];
	}

	if (wf->response_array_len == 0) {// 使用默认response list
		sr = kmalloc(sizeof(struct secDetector_response) * NR_RESPONSE, GFP_KERNEL);
		if (sr == NULL) {
			pr_err("kmalloc failed");
			return;
		}
		for (i = 0; i < NR_RESPONSE; i++) {
			sr[i].response_type = i;
			sr[i].response_func = response_units[i];
		}
	} else {// 自定义response list
		for (i = 0; i < wf->response_array_len; i++) {
			if (sr[i].response_type < RESPONSE_CUSTOMIZATION)
			sr[i].response_func = response_units[sr[i].response_type];
		}
	}

}

int secDetector_module_register(struct secDetector_module *module)
{
	struct secDetector_workflow *wf = NULL;
	int ret = 0;
	int i;
	int module_id;
	unsigned int callback_id = 0;

	if (module == NULL) {
		pr_err("[secDetector] register module is null\n");
		return -EINVAL;
	}

	module_id = idr_alloc(&g_module_idr, module, 0, INT_MAX, GFP_KERNEL);
	if (module_id < 0) {
		pr_err("[secDetector] alloc module id failed\n");
		return module_id;
	}

	mutex_lock(&g_hook_list_array_mutex);
	for (i = 0, wf = module->workflow_array; i < module->workflow_array_len;
		 i++, wf++) {
		if (wf == NULL) {
			ret = -EINVAL;
			goto error;
		}
		wf->module = module;
		if (wf->workflow_type == WORKFLOW_PRESET) {
			wf->workflow_func.func_wf = preset_workflow;
			secDetector_collect_analyze_response_unit_padding(wf);
		}

		ret = insert_callback(wf);
		if (ret < 0) {
			pr_err("[secDetector] insert callback failed\n");
			goto error;
		}
		wf->id = callback_id++;
	}

	module->id = (unsigned int)module_id;
	list_add_rcu(&module->list, &secDetector_module_list);
	mutex_unlock(&g_hook_list_array_mutex);

	return ret;

error:
	mutex_unlock(&g_hook_list_array_mutex);
	secDetector_module_unregister(module);
	return ret;
}
EXPORT_SYMBOL_GPL(secDetector_module_register);

void secDetector_init_manager(void)
{
	init_secDetector_hook();
}
