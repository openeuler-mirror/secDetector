/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * create: 2023-10-11
 * Description: the analyze unit of save check head.
 */
#ifndef SECDETECTOR_SAVE_CHECK_H
#define SECDETECTOR_SAVE_CHECK_H
#include "secDetector_analyze_type.h"
#include "secDetector_collect_type.h"
int analyze_save_check(struct list_head *collect_data_list, analyze_status_t *analyze_status_data, response_data_t *response_data);

void free_analyze_status_data_sc(analyze_status_t *analyze_status_data);
#endif