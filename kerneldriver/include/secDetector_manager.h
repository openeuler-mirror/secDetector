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
 */


#ifndef SECDETECTOR_MANAGER_H
#define SECDETECTOR_MANAGER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include "secDetector_module_type.h"

extern void secDetector_init_manager(void);
extern int secDetector_module_register(struct secDetector_module *module);
extern void secDetector_module_unregister(struct secDetector_module *module);

#endif
