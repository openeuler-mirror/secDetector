/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: yieux
 * Create: 2023-09-25
 * Description: secDetector analyze unit type header
 */
#ifndef SECDETECTOR_ANALYZE_TYPE_H
#define SECDETECTOR_ANALYZE_TYPE_H

enum ANALYZE_TYPE {
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