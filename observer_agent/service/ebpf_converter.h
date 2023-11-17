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
 * Create: 2023-11-16
 * Description: ebpf converter
 */
#ifndef SECDETECTOR_EBPF_CONVERTER_H
#define SECDETECTOR_EBPF_CONVERTER_H
#include "ebpf/ebpf_types.h"

std::string ebpf_event_to_text(struct ebpf_event *e);
#endif
