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
 * Author: zcfsite
 * create: 2023-09-22
 * Description: secDetector proc log 
 */

#include <linux/errno.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "secDetector_proc.h"

#define ALARM_RECORDS_MAX_SIZE 4096
#define ALARM_FILE "alarm"
#define ALARM_FILE_MASK 0600
#define MSG_ALIGN 4
#define MSG_LEN 4096

static struct proc_dir_entry *g_proc_alarm;
s_secDetector_log *g_secDetector_log_fifo = NULL;

static int clean_log_fifo_data(void)
{
	int ret;
	unsigned int len;
	s_log_data log = {0, NULL};

	if (!g_secDetector_log_fifo || !g_secDetector_log_fifo->inflag)
		return 0;
	
	g_secDetector_log_fifo->inflag = false;
	len = kfifo_len(&g_secDetector_log_fifo->log_fifo);
	while (len != 0) {
		ret = kfifo_out_spinlocked(&g_secDetector_log_fifo->log_fifo, &log, 1, &g_secDetector_log_fifo->log_fifo_lock);
		if (ret != 1) {
			g_secDetector_log_fifo->inflag = true;
			return -1;
		}
		len = kfifo_len(&g_secDetector_log_fifo->log_fifo);
		kfree(log.data);
	}

	g_secDetector_log_fifo->inflag = true;
	return 0;
}

static int check_and_out_log_fifo(void)
{
	int ret;
	unsigned int avail_len;
	s_log_data log = {0, NULL};

	avail_len = kfifo_len(&g_secDetector_log_fifo->log_fifo);
	while (avail_len == 0) {
                ret = kfifo_out_spinlocked(&g_secDetector_log_fifo->log_fifo, &log, 1, &g_secDetector_log_fifo->log_fifo_lock);
                if (ret != 1) 
                        return -1;
		avail_len = kfifo_len(&g_secDetector_log_fifo->log_fifo);
		kfree(log.data);
	}
	return 0;
}

int write_log(const char *buf, unsigned int buf_len)
{
	int ret;
	s_log_data log;
	int len;

	if (!buf || buf_len == 0 || buf_len > MSG_LEN)
		return -EINVAL;
	if (!g_secDetector_log_fifo || !g_secDetector_log_fifo->inflag)
		return -1;
	
	ret  = check_and_out_log_fifo();
	if (ret != 0)
		return -1;
	
	log.data_size = buf_len;
	log.data = (char *) kzalloc(buf_len, GFP_ATOMIC);
	if (!log.data)
		return -1;

	len = (buf_len == MSG_LEN) ? buf_len - 1 : buf_len;
	memcpy(log.data, buf, len);

	ret = kfifo_out_spinlocked(&g_secDetector_log_fifo->log_fifo, &log, 1, &g_secDetector_log_fifo->log_fifo_lock);
	if (ret != 1) {
		kfree(log.data);
		return -1;
	}

	return 0;
}

static int log_data_to_user(char __user *buffer, size_t buflen, s_log_data *log)
{
	int ret;
	if (!log || buflen < log->data_size)
		return -1;
	
	ret = copy_to_user(buffer, log->data, log->data_size);
	if (ret != 0)
		return -EFAULT;

	return log->data_size;
}

static ssize_t secDetector_log_read(struct file *file, char __user *buffer, size_t buflen, loff_t *fpos)
{
	int ret;
	unsigned int read_len;
	bool empty = false;
	s_log_data log = {0, NULL};

	if ((buflen == 0) || (buflen % MSG_ALIGN)) 
		return -EINVAL;

	empty = kfifo_is_empty(&g_secDetector_log_fifo->log_fifo);
	if (empty)
		return 0;

	read_len = kfifo_out_spinlocked(&g_secDetector_log_fifo->log_fifo, &log, 1, &g_secDetector_log_fifo->log_fifo_lock);
	if (read_len != 1)
		return -1;

	ret = log_data_to_user(buffer, buflen, &log);
	if (log.data)
		kfree(log.data);

	return ret;
}

static ssize_t secDetector_log_write(struct file *filp, const char __user *buffer, size_t count, loff_t *ppos)
{
	int ret;
	if (count != 1)
		return (ssize_t)-EINVAL;

	ret = clean_log_fifo_data();
	if (ret != 0)
		return ret;

	return count;
}

static const struct proc_ops g_proc_log_file_operations = {
	.proc_read = secDetector_log_read,
	.proc_write = secDetector_log_write,
	.proc_lseek = no_llseek,
};

static int create_log(size_t log_size, struct proc_dir_entry *parent)
{
	int ret;
	unsigned int avail_len;

	g_secDetector_log_fifo = kzalloc(sizeof(s_secDetector_log), GFP_KERNEL);
	if (!g_secDetector_log_fifo)
		return -1;
	
	spin_lock_init(&g_secDetector_log_fifo->log_fifo_lock);
	g_secDetector_log_fifo->inflag = true;

	INIT_KFIFO(g_secDetector_log_fifo->log_fifo);
	ret = kfifo_alloc(&g_secDetector_log_fifo->log_fifo, log_size, GFP_KERNEL);
	if (ret != 0) {
		kfree(g_secDetector_log_fifo);
		g_secDetector_log_fifo = NULL;
		return -1;
	}

	avail_len = kfifo_avail(&g_secDetector_log_fifo->log_fifo);
	g_proc_alarm = proc_create(ALARM_FILE, ALARM_FILE_MASK, parent, &g_proc_log_file_operations);
	if (!g_proc_alarm) {
		kfifo_free(&g_secDetector_log_fifo->log_fifo);
		kfree(g_secDetector_log_fifo);
		g_secDetector_log_fifo = NULL;
		return -1;
	}

	return 0;
}

void secDetector_destroy_log(void)
{
	int ret;
	if (g_proc_alarm) {
		proc_remove(g_proc_alarm);
		g_proc_alarm = NULL;
	}

	if (g_secDetector_log_fifo) {
		ret = clean_log_fifo_data();
		if (ret != 0)
			return;
		kfifo_free(&g_secDetector_log_fifo->log_fifo);
		kfree(g_secDetector_log_fifo);
		g_secDetector_log_fifo = NULL;
	}
}

int __init secDetector_init_log(struct proc_dir_entry *parent, size_t log_size)
{
	int ret;

	if (!parent || log_size <= 1 || log_size > ALARM_RECORDS_MAX_SIZE)
		return -EINVAL;

	if (log_size != roundup_pow_of_two(log_size)) {
		pr_err("[secDetector] init log_size roundup error, suggest log_size=%zu\n", roundup_pow_of_two(log_size));
		return -EINVAL;
	}

	ret = create_log(log_size, parent);
	if (ret != 0) {
		pr_err("[secDetector] init log error");
		return ret;
	}

	return ret;
}
