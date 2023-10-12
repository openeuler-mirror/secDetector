/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector response source
 */
#include <linux/printk.h>
#include <linux/bpf.h>
#include <linux/slab.h>
#include "secDetector_response.h"
#include "secDetector_proc.h"
#include "secDetector_ringbuffer.h"

response_func_t response_units[NR_RESPONSE] = {
	[RESPONSE_OK] = secDetector_ok,
	[RESPONSE_REPORT] = secdetector_report,
};

void notrace secdetector_respond(unsigned int response_type,
				 response_data_t *data)
{
	if (response_type >= NR_RESPONSE)
		return;

	response_units[response_type](data);
}
EXPORT_SYMBOL_GPL(secdetector_respond);

void notrace secDetector_ok(response_data_t *data)
{
	return;
}

void notrace secdetector_report(response_data_t *log)
{
	int ret;

	if (!log || !log->report_data.text)
		return;

	ret = secDetector_ringbuf_output(log->report_data.text,
					 log->report_data.len,
					 BPF_RB_FORCE_WAKEUP);
	if (ret != 0)
		pr_warn("write ringbuf failed\n");
}
EXPORT_SYMBOL_GPL(secdetector_report);

void notrace secDetector_proc_report(response_data_t *log)
{
	int ret;
	if (!log || !log->report_data.text)
		return;

	ret = write_log(log->report_data.text, log->report_data.len);
	if (ret != 0)
		pr_warn("write_log failed");
}
EXPORT_SYMBOL_GPL(secDetector_proc_report);




void free_response_data_no_rd(uint32_t repsonse_id, response_data_t *rd)
{
	if (rd == NULL)
		return;
	switch (repsonse_id) {
		case RESPONSE_REPORT:
			if (rd->report_data.len != 0 && rd->report_data.text != NULL)
				kfree(rd->report_data.text);
			break;
		default:
			break;
	}
}

void free_response_data(uint32_t repsonse_id, response_data_t *rd)
{
	if (rd == NULL)
		return;
	switch (repsonse_id) {
		case RESPONSE_REPORT:
			if (rd->report_data.len != 0 && rd->report_data.text != NULL) {
				kfree(rd->report_data.text);
				kfree(rd);
			}
			break;
		default:
			break;
	}
}
