/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-11
 * Description: the analyze unit of save check func.
 */
#include "secDetector_analyze.h"
#include "secDetector_collect_type.h"
#include "secDetector_response_type.h"
#include <linux/crypto.h>
#include <linux/hash.h>
#include <linux/err.h>
#include <crypto/sha2.h>
//"[save_check]" + ": original:" + MAX_DIGITS +"; now: "+ MAX_DIGITS +".!\n" 33 + 2*MAX_DIGITS + 1
#define REPORT_MORE_CHAR_LEN 80
#define MAX_DIGITS 20

static unsigned long long hash_to_long(void *data, int len)
{
	char hash[SHA256_DIGEST_SIZE];
	int i;
	unsigned long long ret = 0;

	sha256(data, len, hash);
	ret = *(unsigned long long *)hash;
	for (i = 0; i < SHA256_DIGEST_SIZE; i += sizeof(unsigned long long)) {
		ret = ret & (*(unsigned long long *)(hash + i));
	}

	return ret;
}

static int init_analyze_status_data_sc(analyze_status_t *analyze_status_data, int len)
{
	if (analyze_status_data == NULL) {
		pr_err("invalid analyze_status_data!");
		return 0;
	}
	analyze_status_data->sc_data.data = kmalloc(sizeof(unsigned long long) * len, GFP_KERNEL);
	if (analyze_status_data->sc_data.data == NULL) {
		pr_err("kmalloc failed");
		return -ENOMEM;
	}
	analyze_status_data->sc_data.data_type = ANALYZE_STATUS_SAVE_CHECK;
	analyze_status_data->sc_data.len = len;
	return 0;
}


void free_analyze_status_data_sc(analyze_status_t *analyze_status_data)
{
	if (analyze_status_data != NULL && analyze_status_data->sc_data.data != NULL)
		kfree(analyze_status_data->sc_data.data);
}

static int analyze_save_check_init(struct list_head *collect_data_list, analyze_status_t *analyze_status_data, response_data_t *response_data)
{
	int ret = 0;
	int data_index = 0;
	struct collect_data *cd;
	list_for_each_entry(cd, collect_data_list, list) {
		if (cd->name == NULL)
			continue;
		data_index++;
	}
	ret = init_analyze_status_data_sc(analyze_status_data, data_index);
	if (ret < 0)
		return ret;

	data_index = 0;
	list_for_each_entry(cd, collect_data_list, list) {
		if (cd->name == NULL)
			continue;
		switch (cd->value_type) {
			case COLLECT_VALUE_INT:
				analyze_status_data->sc_data.data[data_index] = cd->value.int_value;
				break;
			case COLLECT_VALUE_REGION:
				analyze_status_data->sc_data.data[data_index] = hash_to_long(cd->value.region_value.addr, cd->value.region_value.size);
				break;
			default:
				break;
		}
		data_index ++;
	}

	analyze_status_data->sc_data.init_tag = 1;
	return RESPONSE_OK;
}



static int analyze_save_check_normal(struct list_head *collect_data_list, analyze_status_t *analyze_status_data, response_data_t *response_data, unsigned int event_type)
{
	int data_index = 0;
	unsigned long long measure_value;
	struct collect_data *cd;
	char *timestamp = NULL;
	int timestamp_len = 0;
	char **response_arrays;
	int response_array_index = 0;
	char int_str[MAX_DIGITS];
	uint32_t response_data_char_len = 0;
	int ret = RESPONSE_OK;
	int i;

	list_for_each_entry(cd, collect_data_list, list) {
		if (cd->name == NULL)
			continue;
		data_index++;
	}
	response_arrays = kmalloc(sizeof(char *) * data_index, GFP_KERNEL);
	if (response_arrays == NULL) {
		pr_err("kmalloc failed");
		return -ENOMEM;
	}

	data_index = 0;
	list_for_each_entry(cd, collect_data_list, list) {
		if (cd->name == NULL)
			continue;
		switch (cd->value_type) {
			case COLLECT_VALUE_INT:
				measure_value = cd->value.int_value;
				break;
			case COLLECT_VALUE_REGION:
				measure_value = hash_to_long(cd->value.region_value.addr, cd->value.region_value.size);
				break;
			default:
				measure_value = 0;
				break;
		}
		if (measure_value != analyze_status_data->sc_data.data[data_index]) {
			pr_debug("[save_check]%s: original: %llu; now: %llu.!\n",
					cd->name, analyze_status_data->sc_data.data[data_index], measure_value);
			response_arrays[response_array_index] = kzalloc(strlen(cd->name) + REPORT_MORE_CHAR_LEN, GFP_KERNEL);
			if (response_arrays[response_array_index] == NULL) {
				pr_err("kzalloc failed");
				ret = -ENOMEM;
				goto end;
			}

			strcpy(response_arrays[response_array_index], " secswitch_name=");
			//应该有 workflow的名字
			strncat(response_arrays[response_array_index], cd->name, strlen(cd->name));
			strcat(response_arrays[response_array_index], " old_value=");
			sprintf(int_str, "%llu", analyze_status_data->sc_data.data[data_index]);
			strncat(response_arrays[response_array_index], int_str, strlen(int_str));
			strcat(response_arrays[response_array_index], " new_value=");
			sprintf(int_str, "%llu", measure_value);
			strncat(response_arrays[response_array_index], int_str, strlen(int_str));
			strcat(response_arrays[response_array_index], ".");

			response_data_char_len += strlen(response_arrays[response_array_index]);
			ret = RESPONSE_REPORT;
			response_array_index ++;
			analyze_status_data->sc_data.data[data_index] = measure_value;
		}
		data_index ++;
	}

	if (ret == RESPONSE_REPORT) {
		timestamp_len = get_timestamp_str(&timestamp);
		response_data->report_data.type = event_type;
		response_data->report_data.len = response_data_char_len + timestamp_len;
		response_data->report_data.text = kzalloc(response_data->report_data.len + 1, GFP_KERNEL);
		if (response_data->report_data.text == NULL) {
			pr_err("kzalloc failed");
			ret = -ENOMEM;
			goto end;
		}

		for (i = 0; i < response_array_index; i++)
			strncat(response_data->report_data.text, response_arrays[i], strlen(response_arrays[i]));
		strcat(response_data->report_data.text, "\n");
	}

end:
	if (timestamp_len > 0) {
		if (response_data->report_data.text)
			strncat(response_data->report_data.text, timestamp, timestamp_len);
		kfree(timestamp);
	}
	for (i = 0; i < response_array_index; i++)
		kfree(response_arrays[i]);
	kfree(response_arrays);

	return ret;
}

int analyze_save_check(struct list_head *collect_data_list, analyze_status_t *analyze_status_data, response_data_t *response_data, unsigned int event_type)
{
	if (analyze_status_data->sc_data.init_tag == 1) {
		return analyze_save_check_normal(collect_data_list, analyze_status_data, response_data, event_type);
	} else {
		return analyze_save_check_init(collect_data_list, analyze_status_data, response_data);
	}
	return 0;
}


