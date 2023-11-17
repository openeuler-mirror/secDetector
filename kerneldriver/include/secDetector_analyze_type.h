/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector analyze unit type header
 */
#ifndef SECDETECTOR_ANALYZE_TYPE_H
#define SECDETECTOR_ANALYZE_TYPE_H
#include <linux/list.h>
#include "secDetector_response_type.h"

enum ANALYZE_TYPE {
	ANALYZE_RECORD,
	ANALYZE_PRESET_START,
	ANALYZE_PRESET_SAVE_CHECK = ANALYZE_PRESET_START,
	ANALYZE_PRESET_FREQUENCY_RANGE,
	ANALYZE_PRESET_END = ANALYZE_PRESET_FREQUENCY_RANGE,
	ANALYZE_CUSTOMIZATION,
	NR_ANALYZE,
};

enum ANALYZE_STATUS_DATA_TYPE {
	ANALYZE_STATUS_BASIC,
	ANALYZE_STATUS_SAVE_CHECK,
};

struct analyze_basic_data {
	uint32_t data_type;
	void *data;
	uint32_t len;
};

struct save_check_data {
	uint32_t data_type;
	unsigned long long *data;
	uint32_t len;
	uint32_t init_tag;

};

typedef union analyze_status {
	struct analyze_basic_data data;
	struct save_check_data sc_data;
} analyze_status_t;

// 暂时没有对多类型支持需求
// typedef union analyze_func {
// 	void (*null_func)(void);
// 	int (*func)(struct list_head *, analyze_status_t *);
// 	void (*analyze_record_func)(void);
// 	int (*save_check_func)(struct list_head *, analyze_status_t *);
// } analyze_func_t;

typedef int (*analyze_func_t)(struct list_head *, analyze_status_t *, response_data_t *, unsigned int);
#endif