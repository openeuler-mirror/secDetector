/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
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
}
extern void notrace secdetector_respond(unsigned int response_type, response_data_t *data);
extern void notrace secdetector_report(response_data_t *data);

// support max 4095 bytes,
extern void notrace secDetector_proc_report(response_data_t *log);
#endif
