/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-11-17
 * Description: the collect unit of time head.
 */
#ifndef SECDETECTOR_TIME_H
#define SECDETECTOR_TIME_H
#include "secDetector_collect_type.h"
void collect_time(current_context_t cc, struct list_head *ret_list);
#endif