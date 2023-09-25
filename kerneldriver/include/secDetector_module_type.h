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
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector module type header
 */
#ifndef SECDETECTOR_MODULE_TYPE_H
#define SECDETECTOR_MODULE_TYPE_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include "secDetector_workflow_type.h"

typedef struct secDetector_workflow secDetector_workflow_t;
struct secDetector_module {
    struct list_head list;
    struct rcu_head rcu;
    unsigned int id;
    char *name;
    struct module *kmodule;
    atomic_t enabled;

    secDetector_workflow_t *workflow_array;
    uint32_t workflow_array_len;
};

#endif