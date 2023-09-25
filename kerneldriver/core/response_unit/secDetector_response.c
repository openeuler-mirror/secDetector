/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector response source
 */
#include <linux/printk.h>
#include "secDetector_response.h"
#include "secDetector_proc.h"

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
	pr_info("%s", data->report_data->text);
}

void notrace secDetector_proc_report(response_data_t *log)
{
	int ret;
	if (!log || !log->report_data || !log->report_data->text)
		return;

	ret = write_log(log->report_data->text, log->report_data->len);
	if (ret != 0)
		pr_warn("write_log failed");
}
EXPORT_SYMBOL_GPL(secDetector_proc_report);
