/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-11
 * Description: the analyze unit func.
 */
#include "secDetector_analyze.h"
#include "secDetector_save_check.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <time.h>
#define TIME_STR_MAX_LEN 100

analyze_func_t analyze_units[NR_ANALYZE] = {
	[ANALYZE_PRESET_SAVE_CHECK] = analyze_save_check,
};

// 不使用analyze_status_data的时候，data_type 为0，因此free_analyze_status_data不处理对应的 ANALYZE_STATUS。
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

int get_timestamp_str(char **ret_str)
{
	struct timespec64 ts;
	struct tm stm;
	char *stm_str = NULL;
	int stm_str_len = 0;

	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, 0, &stm);

	stm_str = (char *)kzalloc(TIME_STR_MAX_LEN, GFP_ATOMIC);
	if (stm_str == NULL) {
		pr_err("kzalloc failed\n");
		*ret_str = NULL;
		return 0;
	}

	stm_str_len = scnprintf(stm_str, TIME_STR_MAX_LEN,
			"timestamp=%04ld%02d%02d.%02d:%02d:%02d ",
			stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
	if (stm_str_len <= 0) {
		pr_err("scnprintf failed\n");
		kfree(stm_str);
		*ret_str  = NULL;
		return 0;
	}

	*ret_str = kstrdup(stm_str, GFP_KERNEL);
	if (*ret_str == NULL) {
		pr_err("kstrdup failed\n");
		stm_str_len = 0;
	}

	kfree(stm_str);
	return stm_str_len;
}
EXPORT_SYMBOL_GPL(get_timestamp_str);
