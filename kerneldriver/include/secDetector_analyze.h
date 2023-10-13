/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector analyze unit header
 */
#ifndef SECDETECTOR_ANALYZE_H
#define SECDETECTOR_ANALYZE_H
#include "secDetector_analyze_type.h"

extern analyze_func_t analyze_units[NR_ANALYZE];

void free_analyze_status_data(analyze_status_t *analyze_status_data);
#endif