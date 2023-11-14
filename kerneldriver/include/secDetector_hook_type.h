/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector hook unit type header
 */
#ifndef SECDETECTOR_HOOK_TYPE_H
#define SECDETECTOR_HOOK_TYPE_H

#include <linux/kprobes.h>

enum HOOK_TYPE {
	KPROBE_HOOK_START,
	KPROBE_VFS_UNLINK = KPROBE_HOOK_START,
	KPROBE_MEMFD_CREATE,
	KPROBE_HOOK_END = KPROBE_MEMFD_CREATE,

	LSM_HOOK_START,
	LSM_INODE_MKDIR = LSM_HOOK_START,
	LSM_HOOK_END = LSM_INODE_MKDIR,

	TRACEPOINT_HOOK_START,
	TRACEPOINT_TASK_EVENT = TRACEPOINT_HOOK_START,
	TRACEPOINT_FILE_EVENT,
	TRACEPOINT_HOOK_END = TRACEPOINT_FILE_EVENT,

	HOOKEND,

	SECDETECTOR_TIMER,
};

#endif
