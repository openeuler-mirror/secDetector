/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-10
 * Description: kallsyms lookup name func
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
#include <linux/kprobes.h>
#include <linux/mm_types.h>
#include <asm-generic/errno.h>
#include <asm-generic/sections.h>
#include <asm/tlbflush.h>

#include "hide_utils.h"

#define GETBUF_LEN 2
static long unsigned int g_symbol_lookup_addr = 0;
SYMBOL_LOOKUP_FUNC kallsyms_lookup_name_ex;

static int test_find_kernel_symbol(uint64_t addr, char *buf, size_t *offset, size_t *size)
{
	int len = 0, name_len = 0;
	char tmp_buf[KSYM_SYMBOL_LEN] = {0};

	len = sprint_symbol(tmp_buf, addr);
	while (name_len < len && tmp_buf[name_len] != '+')
		name_len++;

	if (name_len == len) {
		pr_err("test name_len error\n");
		return -1;
	}

	if (sscanf(tmp_buf + name_len + 1, "%lx/%lx", offset, size) < GETBUF_LEN) {
		pr_err("test sscanf failed\n");
		return -1;
	}

	name_len = name_len < KSYM_SYMBOL_LEN ? name_len : KSYM_SYMBOL_LEN - 1;
	strncpy(buf, tmp_buf, name_len);

	return 0;
}

static int test_get_symbol_lookup_func(void)
{
	const char *symbol_name = "kallsyms_lookup_name";
	uint64_t kaddr = (unsigned long) &sprint_symbol;
	uint64_t prev = kaddr - 1;
	uint64_t next = kaddr;
	int try_num = 100;

	for (; try_num > 0; try_num--) {
		size_t offset, size;
		char symbol_buffer[KSYM_SYMBOL_LEN] = {0};
		if (next > 0) {
			if (test_find_kernel_symbol(next, symbol_buffer, &offset, &size) != 0)
				break;
			if (strcmp(symbol_buffer, symbol_name) == 0) {
				g_symbol_lookup_addr = next - offset;
				break;
			}
			if (offset > size)
				break;
			next = next + size - offset;
		}

		if (prev > 0) {
			if(test_find_kernel_symbol(prev, symbol_buffer, &offset, &size) != 0)
				break;

			if (strcmp(symbol_buffer, symbol_name) == 0) {
				g_symbol_lookup_addr = prev -offset;
				break;
			}
			if (offset > size)
				break;
			prev = prev - offset - 1;
		}
	}
	
	if (g_symbol_lookup_addr != 0) {
		kallsyms_lookup_name_ex = (SYMBOL_LOOKUP_FUNC)g_symbol_lookup_addr;
		return 0;
	} else {
		pr_err("find symbol address failed\n");
		return -1;
	}
}

int init_symbol_lookup_func(void)
{
	if (test_get_symbol_lookup_func() != 0) {
		pr_err("test get symbol lookup func failed\n");
		return -1;
	}
	return 0;
}
