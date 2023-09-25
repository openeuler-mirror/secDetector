/*
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef SECDETECTOR_PROC_H
#define SECDETECTOR_PROC_H
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kfifo.h>

typedef struct secDetector_log_data {
	unsigned int data_size;
	char *data;
} s_log_data;

typedef struct secDetector_log {
	DECLARE_KFIFO_PTR(log_fifo, s_log_data);
	spinlock_t log_fifo_lock;
	bool inflag;
} s_secDetector_log;

extern int __init secDetector_init_log(struct proc_dir_entry *parent, size_t log_size);
extern void secDetector_destroy_log(void);
extern int write_log(const char*buf, unsigned int buf_len);

#endif
