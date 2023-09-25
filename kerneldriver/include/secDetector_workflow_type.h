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
 * Description: secDetector workflow type header
 */
#ifndef SECDETECTOR_WORKFLOW_TYPE_H
#define SECDETECTOR_WORKFLOW_TYPE_H
#include "secDetector_hook_type.h"
#include "secDetector_collect_type.h"
#include "secDetector_analyze_type.h"
#include "secDetector_response_type.h"
#include "secDetector_module_type.h"

enum WORKFLOW_TYPE{
    WORKFLOW_CUSTOMIZATION,
    WORKFLOW_PRESET,
};

union workflow_func {
    void (*func)(void);
    void (*create_file)(struct filename *);
    void (*write_file)(struct filename *);
    void (*create_process)(int);
    void (*timer_func)(struct timer_list *);
};

typedef struct secDetector_module secDetector_module_t;
typedef struct secDetector_workflow {
    struct list_head list;
    struct rcu_head rcu;
    unsigned int id;
    secDetector_module_t *module;
    atomic_t enabled;
    unsigned int workflow_type;
    union workflow_func workflow_func;

    //hook
    unsigned int hook_type;
    int interval;

    //collect
    struct secDetector_collect *collect_array;
    uint32_t collect_array_len;

    //analyze
    unsigned int analyze_type;
    analyze_func_t analyze_func;

    //response
    struct secdetector_response *response_array;
    uint32_t response_array_len;

} secDetector_workflow_t;



#endif