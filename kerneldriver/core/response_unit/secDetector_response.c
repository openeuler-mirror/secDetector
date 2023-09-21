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
 * Description: secDetector response source
 */
#include "secDetector_response.h"

static const response_func_t response_units[NR_RESPONSE] = {
	[RESPONSE_REPORT] = secdetector_report,
};

void notrace secdetector_respond(unsigned int response_type, response_data_t *data)
{
	if (response_type >= NR_RESPONSE)
		return;

	response_units[response_type](data);
}

void notrace secdetector_report(response_data_t *data)
{
	pr_info("%s", data->report_data.text);
}