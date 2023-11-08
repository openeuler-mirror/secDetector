/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-09-25
 * Description: detector workflow manager
 */
#include "secDetector_workflow_type.h"
#include "secDetector_collect.h"
#include "secDetector_response.h"

static void free_collect_data_list(struct list_head *head)
{
	struct collect_data *node, *next;
	list_for_each_entry_safe(node, next, head, list) {
		list_del(&node->list);
		free_collect_data(node);
	}
}

void preset_workflow(secDetector_workflow_t *wf)
{
	LIST_HEAD(collect_data_list);
	int i;
	uint32_t repsonse_id;
	struct secDetector_collect *sc = wf->collect_array;

	response_data_t rd;
	
	for (i = 0; i < wf->collect_array_len; i++, sc++) {
		sc->collect_func(NULL, &collect_data_list);
	}

	repsonse_id = wf->analyze_func(&collect_data_list, &(wf->analyze_status), &rd);

	if (repsonse_id >= 0) {
		wf->response_array[repsonse_id].response_func(&rd);
	}
	
	free_collect_data_list(&collect_data_list);
	free_response_data_no_rd(repsonse_id, &rd);
}