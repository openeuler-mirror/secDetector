/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-10
 * Description: test for hiding kmodule
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm-generic/errno.h>
#include <asm/tlbflush.h>

#include "hide_kmodule.h"

static struct list_head *g_module_prev = NULL;

void hide_self(void)
{
	g_module_prev = THIS_MODULE->list.prev;
	list_del_init(&THIS_MODULE->list);
	pr_info("hide kmodule self ok\n");
}

void unhide_self(void)
{
	if (g_module_prev) {
		list_add(&THIS_MODULE->list, g_module_prev);
		g_module_prev = NULL;
	}
}
