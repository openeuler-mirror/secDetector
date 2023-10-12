/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-11
 * Description: the analyze unit func.
 */
#include "secDetector_analyze.h"
#include "secDetector_save_check.h"


analyze_func_t analyze_units[NR_ANALYZE] = {
	[ANALYZE_PRESET_SAVE_CHECK] = analyze_save_check,
};

void free_analyze_status_data(analyze_status_t *analyze_status_data)
{
	switch (analyze_status_data->data.data_type) {
		case ANALYZE_STATUS_SAVE_CHECK:
			free_analyze_status_data_sc(analyze_status_data);
			break;
		default:
			break;
	}
}