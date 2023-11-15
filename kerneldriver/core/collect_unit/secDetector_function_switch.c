/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-08
 * Description: the collect unit of function switch func.
 */
#include "secDetector_function_switch.h"
#include "secDetector_collect.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#if defined(__x86_64__)
#include <asm/processor-flags.h>
#endif
#if defined(__aarch64__)
#include <asm/sysreg.h>
#endif



static void collect_SMEP_tag(struct list_head *ret_list)
{
	unsigned long long value;
	#if defined(__x86_64__)
	const char *name = "SMEP";
	#else
	const char *name = NULL;
	#endif
	struct collect_data *cd = init_collect_data(name);
	if (cd == NULL)
		return;

	#if defined(__x86_64__)
	asm volatile ("mov %%cr4, %0":"=r" (value));
	value = (value & X86_CR4_SMEP)? 1 : 0;
	#endif

	cd->value_type = COLLECT_VALUE_INT;
	cd->value.int_value = value;

	list_add_tail(&cd->list, ret_list);
}

static void collect_SMAP_tag(struct list_head *ret_list)
{
	unsigned long long value;
	#if defined(__x86_64__)
	const char *name = "SMAP";
	#else
	const char *name = NULL;
	#endif
	struct collect_data *cd = init_collect_data(name);
	if (cd == NULL)
		return;

	#if defined(__x86_64__)
	asm volatile ("mov %%cr4, %0":"=r" (value));
	value = (value & X86_CR4_SMAP)? 1 : 0;
	#endif

	cd->value_type = COLLECT_VALUE_INT;
	cd->value.int_value = value;

	list_add_tail(&cd->list, ret_list);
}


static void collect_WP_tag(struct list_head *ret_list)
{
	unsigned long long value;
	#if defined(__x86_64__)
	const char *name = "WP";
	#else
	const char *name = NULL;
	#endif
	struct collect_data *cd = init_collect_data(name);
	if (cd == NULL)
		return;

	#if defined(__x86_64__)
	asm volatile ("mov %%cr0, %0":"=r" (value));
	value = (value & X86_CR0_WP)? 1 : 0;
	#endif

	cd->value_type = COLLECT_VALUE_INT;
	cd->value.int_value = value;

	list_add_tail(&cd->list, ret_list);
}

static void collect_WXN_tag(struct list_head *ret_list)
{
	unsigned long long value;
	#if defined(__aarch64__)
	const char *name = "WXN";
	#else
	const char *name = NULL;
	#endif
	struct collect_data *cd = init_collect_data(name);
	if (cd == NULL)
		return;

	#if defined(__aarch64__)
	asm volatile ("mrs %0, sctlr_el1" : "=r" (value));
	value = (value & (1UL << 19))? 1 : 0;
	#endif

	cd->value_type = COLLECT_VALUE_INT;
	cd->value.int_value = value;

	list_add_tail(&cd->list, ret_list);
}



void collect_function_switch(current_context_t cc, struct list_head *ret_list)
{
	collect_SMEP_tag(ret_list);
	collect_SMAP_tag(ret_list);
	collect_WP_tag(ret_list);
	collect_WXN_tag(ret_list);
}


// 	#if defined(__i386__) || defined(__x86_64__)
// 	asm volatile ("mov %%cr4, %0":"=r" (value));
// 	value = (value & X86_CR4_SMEP)? 1 : 0;
// 	#elif defined(__arm__)
// 	asm volatile ("mcr p15, 0, %0, c0, c0, 0" : "=r" (value));
// 	value = (value & (1UL << 29))? 1 : 0;
// 	#elif defined(__aarch64__)
// 	asm volatile ("mrs %0 sctlr_el1" : "=r" (value));
// 	value = (value & (1UL << 29))? 1 : 0;
// 	#endif
