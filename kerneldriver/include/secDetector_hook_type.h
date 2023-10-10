/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector hook unit type header
 */
#ifndef SECDETECTOR_HOOK_TYPE_H
#define SECDETECTOR_HOOK_TYPE_H

enum HOOK_TYPE {
	TRACEPOINT_HOOK_START,
	TRACEPOINT_FILE_EVENT = TRACEPOINT_HOOK_START,
	TRACEPOINT_HOOK_END = TRACEPOINT_FILE_EVENT,

	HOOKEND,

	SECDETECTOR_TIMER,
};

#endif
