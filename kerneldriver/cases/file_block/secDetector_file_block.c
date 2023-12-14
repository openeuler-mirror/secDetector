/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * create: 2023-10-25
 * Description: the main implement of file block
 */
#include <linux/module.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/glob.h>
#include <linux/rculist.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "secDetector_manager.h"
#include "secDetector_response.h"
#include <secDetector_module_type.h>
#include <string.h>

#define BUF_SIZE 4096
#define RULE_STR_SIZE 1024

struct file_block_rules_item {
	struct list_head list;
	char *rule;
};

static LIST_HEAD(file_block_rule_list);
static DEFINE_MUTEX(rules_mutex);

static int file_write_check(struct secDetector_workflow *wf, struct file *file)
{
	char *buf = NULL;
	char *pathname = NULL;
	response_data_t log;
	bool matched = false;
	struct file_block_rules_item *item = NULL;
	int ret = 0;

	buf = kzalloc(BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return ret;
	pathname = d_path(&file->f_path, buf, BUF_SIZE);
	if (IS_ERR(pathname))
		goto out;

	rcu_read_lock();
	list_for_each_entry_rcu (item, &file_block_rule_list, list) {
		if (glob_match(item->rule, pathname)) {
			matched = true;
			break;
		}
	}
	rcu_read_unlock();

	if (matched) {
		ret = -EPERM;
		log.report_data.len = BUF_SIZE;
		log.report_data.text = kzalloc(BUF_SIZE, GFP_ATOMIC);
		snprintf(log.report_data.text, BUF_SIZE,
			 "[secDetector] %s is not allowed to overwrite\n",
			 pathname);
		secDetector_report(&log);
		kfree(log.report_data.text);
	}

out:
	kfree(buf);
	return ret;
}

static int file_event_handler(struct secDetector_workflow *wf,
			      struct secdetector_file *file, int flag)
{
	if (flag == SECDETECTOR_FILE_WRITE_PRE)
		return file_write_check(wf, file->file);
	return 0;
}

static struct secDetector_workflow workflow_array[] = {
	{ .workflow_type = WORKFLOW_CUSTOMIZATION,
	  .workflow_func.file_event = file_event_handler,
	  .hook_type = TRACEPOINT_FILE_EVENT,
	  .enabled = ATOMIC_INIT(true) },
};

static int proc_show(struct seq_file *m, void *v)
{
	struct file_block_rules_item *item = NULL;

	mutex_lock(&rules_mutex);
	list_for_each_entry (item, &file_block_rule_list, list) {
		seq_printf(m, "%s\n", item->rule);
	}
	mutex_unlock(&rules_mutex);
	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, NULL);
}

static void clear_file_rule_list(void)
{
	struct file_block_rules_item *item = NULL;
	struct file_block_rules_item *tmp = NULL;

	mutex_lock(&rules_mutex);
	list_for_each_entry_safe (item, tmp, &file_block_rule_list, list) {
		list_del_rcu(&item->list);
		synchronize_rcu();
		kfree(item->rule);
		item->rule = NULL;
		kfree(item);
	}
	mutex_unlock(&rules_mutex);
}

static ssize_t proc_write(struct file *file, const char __user *buffer,
			  size_t len, loff_t *offset)
{
	char *data = NULL;
	char *str = NULL;
	char *rule = NULL;
	struct file_block_rules_item *item = NULL;
	ssize_t r = -EINVAL;

	data = memdup_user_nul(buffer, len);
	if (!data)
		return -ENOMEM;

	str = strstrip(data);
	if (strcmp(str, "") == 0) {
		clear_file_rule_list();
		r = len;
	} else {
		str = strsep(&data, "\n");
		rule = kstrdup(str, GFP_KERNEL);
		if (!rule) {
			r = -ENOMEM;
			goto out;
		}
		item = kzalloc(sizeof(*item), GFP_KERNEL);
		if (!item) {
			r = -ENOMEM;
			kfree(rule);
			goto out;
		}
		item->rule = rule;
		mutex_lock(&rules_mutex);
		list_add_tail_rcu(&item->list, &file_block_rule_list);
		mutex_unlock(&rules_mutex);
		r = len;
	}

out:
	kfree(data);
	return r;
}

static const struct proc_ops ops = {
	.proc_open = proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
	.proc_write = proc_write,
};

static struct secDetector_parameter param = {
	.name = "file_block",
	.proc_ops = &ops,
	.mode = 0640,
	.data = NULL,
};

static struct secDetector_module file_block_module = {
	.name = "secDetector file block module",
	.enabled = ATOMIC_INIT(true),
	.workflow_array = workflow_array,
	.workflow_array_len = ARRAY_SIZE(workflow_array),
	.parameter = &param,
};

static int __init register_secDetector_file_block(void)
{
	int ret;
	ret = secDetector_module_register(&file_block_module);
	if (ret < 0)
		pr_err("[secDetector case file block] register failed");
	else
		pr_info("[secDetector case file block] register success\n");

	return ret;
}

static void __exit unregister_secDetector_file_block(void)
{
	clear_file_rule_list();
	(void)secDetector_module_unregister(&file_block_module);

	pr_info("[secDetector case file block] unregister success\n");
}

module_init(register_secDetector_file_block);
module_exit(unregister_secDetector_file_block);
MODULE_LICENSE("GPL");
