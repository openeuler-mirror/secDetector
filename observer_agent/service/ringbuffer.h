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
 * Create: 2023-09-25
 * Description: secDetector userspace ringbuffer header
 */
#ifndef SECDETECTOR_OBSERVER_AGENT_RINGBUFFER_H
#define SECDETECTOR_OBSERVER_AGENT_RINGBUFFER_H
#include <sys/types.h>

typedef int (*poll_cb)(void *sample, size_t len);
struct response_rb_entry
{
    int type;
    char text[];
};

extern int secDetector_ringbuf_attach(void);
extern void secDetector_ringbuf_detach(void);
extern int secDetector_ringbuf_poll(poll_cb cb);
#endif
