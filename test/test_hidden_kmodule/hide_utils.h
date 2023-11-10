/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: zhangguangzhi
 * create: 2023-11-10
 * Description: kallsyms lookup name func
 */

#ifndef HIDE_UTILS_H
#define HIDE_UTILS_H

#include <linux/types.h>

int init_symbol_lookup_func(void);

typedef unsigned long (*SYMBOL_LOOKUP_FUNC)(const char *name);
extern SYMBOL_LOOKUP_FUNC kallsyms_lookup_name_ex;

#endif
