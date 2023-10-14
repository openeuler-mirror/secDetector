/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector response type header
 */
#ifndef SECDETECTOR_REPORT_TYPE_H
#define SECDETECTOR_REPORT_TYPE_H
#include <linux/types.h>

enum RESPONSE_TYPE {
	RESPONSE_OK,
	RESPONSE_REPORT,
	RESPONSE_REJECT,
	RESPONSE_CUSTOMIZATION,
	NR_RESPONSE,
};

struct response_report_data {
	int type;
	char *text;
	size_t len;
};

typedef union response_data {
	struct response_report_data report_data; //这里定义为指针有点多余
} response_data_t;

typedef void (*response_func_t)(response_data_t *data);
#endif