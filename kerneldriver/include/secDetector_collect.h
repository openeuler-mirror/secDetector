/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector collect unit header
 */
#ifndef SECDETECTOR_COLLECT_H
#define SECDETECTOR_COLLECT_H
#include "secDetector_collect_type.h"

extern collect_func_t collect_units[NR_COLLECT];

struct collect_data *init_collect_data(const char *name);
void free_collect_data(struct collect_data *cd);
#endif