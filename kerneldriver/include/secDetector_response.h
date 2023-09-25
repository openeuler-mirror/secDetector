/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-21
 * Description: secDetector reponse header
 */
#ifndef SECDETECTOR_RESPONSE_H
#define SECDETECTOR_RESPONSE_H
#include "secDetector_response_type.h"

struct secdetector_response {
    struct list_head list;
    struct rcu_head rcu;
    unsigned int response_type;
    response_func_t response_func;
};

extern void notrace secdetector_respond(unsigned int response_type, response_data_t *data);
extern void notrace secdetector_report(response_data_t *data);

// support max 4095 bytes,
extern void notrace secDetector_proc_report(response_data_t *log);
#endif
