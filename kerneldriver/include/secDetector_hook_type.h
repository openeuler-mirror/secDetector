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
 * Description: secDetector hook unit type header
 */
#ifndef SECDETECTOR_HOOK_TYPE_H
#define SECDETECTOR_HOOK_TYPE_H

enum HOOK_TYPE {
    TRACEPOINT_HOOK_START,
    TRACEPOINT_CREATE_FILE = TRACEPOINT_HOOK_START,
    TRACEPOINT_WRITE_FILE,
    TRACEPOINT_CREATE_PROCESS,
    TRACEPOINT_HOOK_END = TRACEPOINT_CREATE_PROCESS,

    HOOKEND,

    SECDETECTOR_TIMER,
};

#endif