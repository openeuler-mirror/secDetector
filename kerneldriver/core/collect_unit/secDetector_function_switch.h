/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-08
 * Description: the collect unit of function switch head.
 */
#ifndef SECDETECTOR_FUNCTION_SWITCH_H
#define SECDETECTOR_FUNCTION_SWITCH_H
#include "secDetector_collect_type.h"
void collect_function_switch(current_context_t cc, struct list_head *ret_list);
#endif