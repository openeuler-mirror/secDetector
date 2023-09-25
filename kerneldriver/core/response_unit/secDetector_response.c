/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector response source
 */
#include <linux/printk.h>
#include <linux/bpf.h>
#include "secDetector_response.h"
#include "secDetector_proc.h"
#include "secDetector_ringbuffer.h"

static const response_func_t response_units[NR_RESPONSE] = {
	[RESPONSE_REPORT] = secdetector_report,
};

void notrace secdetector_respond(unsigned int response_type, response_data_t *data)
{
	if (response_type >= NR_RESPONSE)
		return;

	response_units[response_type](data);
}
EXPORT_SYMBOL_GPL(secdetector_respond);

void notrace secdetector_report(response_data_t *log)
{
	int ret;

	if (!log || !log->report_data || !log->report_data->text)
		return;

	ret = secDetector_ringbuf_output(log->report_data->text, log->report_data->len, BPF_RB_FORCE_WAKEUP);
	if (ret != 0)
		pr_warn("write ringbuf failed\n");
}
EXPORT_SYMBOL_GPL(secdetector_report);

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
