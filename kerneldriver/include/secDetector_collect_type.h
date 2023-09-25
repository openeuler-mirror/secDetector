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
 * Description: secDetector collect unit type header
 */
#ifndef SECDETECTOR_COLLECT_TYPE_H
#define SECDETECTOR_COLLECT_TYPE_H

enum COLLECT_TYPE{
	COLLECT_TIME,
	COLLECT_CURRENT_START,
    COLLECT_CURRENT_PROCESS = COLLECT_CURRENT_START,
    COLLECT_CURRENT_FILE,
    COLLECT_CURRENT_END = COLLECT_CURRENT_FILE,
	COLLECT_GLOBAL_START,
    COLLECT_GLOBAL_PROCESS = COLLECT_GLOBAL_START,
    COLLECT_GLOBAL_FILE,
    COLLECT_GLOBAL_RESOURCE,
    COLLECT_GLOBAL_END = COLLECT_GLOBAL_RESOURCE,
    COLLECT_CUSTOMIZATION,
};

union collect_func {
    void (*func)(void);
    void (*COLLECT_record_func)(void);
};

struct secDetector_collect {
	struct list_head list;
	struct rcu_head rcu;
	unsigned int collect_type;
	union collect_func collect_func;
};
#endif