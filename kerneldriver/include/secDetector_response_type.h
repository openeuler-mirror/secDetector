/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector response type header
 */
#ifndef SECDETECTOR_REPORT_TYPE_H
#define SECDETECTOR_REPORT_TYPE_H
#include <linux/types.h>

enum {
	RESPONSE_REPORT,
	RESPONSE_REJECT,
	NR_RESPONSE,
};

struct response_report_data {
	const char *text;
	size_t len;
};

typedef union response_data {
	struct response_report_data *report_data;
} response_data_t;

typedef void (*response_func_t)(response_data_t *data);
#endif