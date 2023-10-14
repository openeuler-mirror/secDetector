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
 * Description: secDetector main entry
 */
#include "ringbuffer.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

static volatile bool exiting = false;
static void sig_handler(int sig) { exiting = true; }

static int ringbuf_cb(struct response_rb_entry *entry, size_t entry_size) {
    if (entry == NULL || entry_size <= sizeof(struct response_rb_entry))
        return -EINVAL;   

    syslog(LOG_INFO, "type:%d, text:%s\n", entry->type, entry->text);
    /* TODO: you can add function there */
    return 0;
}

int main() {
    int r;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    r = daemon(0, 0);
    if (r == -1) {
        printf("daemon failed, r:%d\n");
        exit(EXIT_FAILURE);
    }

    r = secDetector_ringbuf_attach();
    if (r != 0) {
        printf("cannot attach to ringbuffer, r:%d\n", r);
        exit(EXIT_FAILURE);
    }

    while (!exiting) {
        secDetector_ringbuf_poll((poll_cb)ringbuf_cb);
    }

    secDetector_ringbuf_detach();
    return 0;
}