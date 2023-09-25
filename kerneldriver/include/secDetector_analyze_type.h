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
 * Description: secDetector analyze unit type header
 */
#ifndef SECDETECTOR_ANALYZE_TYPE_H
#define SECDETECTOR_ANALYZE_TYPE_H

enum ANALYZE_TYPE{
    ANALYZE_RECORD,
    ANALYZE_PRESET_START,
    ANALYZE_PRESET_SAVE_CHECK = ANALYZE_PRESET_START,
    ANALYZE_PRESET_FREQUENCY_RANGE,
    ANALYZE_PRESET_END = ANALYZE_PRESET_FREQUENCY_RANGE,
    ANALYZE_CUSTOMIZATION,
};

typedef union analyze_func {
    void (*func)(void);
    void (*analyze_record_func)(void);
}analyze_func_t;
#endif