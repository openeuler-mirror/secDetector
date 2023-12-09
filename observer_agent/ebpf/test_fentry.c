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
 * Create: 2023-11-14
 * Description: test prog
 */
#include "ebpf_types.h"
#include "fentry.h"
#include "../../include/secDetector_topic.h"

static int handle_event(void *ctx, void *data, size_t data_sz)
{
    const struct ebpf_event *e = data;
    printf("timestamp:%llu event_name:%s exe:%s pid:%u tgid:%u uid:%u gid:%u comm:%s"
           " sid:%u ppid:%u pgid:%u pcomm:%s nodename:%s pns:%u root_pns:%u",
           e->timestamp, e->event_name, e->exe, e->pid, e->tgid, e->uid, e->gid, e->comm, e->sid, e->ppid, e->pgid,
           e->pcomm, e->nodename, e->pns, e->root_pns);
    if (e->type & (CREATEFILE | DELFILE | SETFILEATTR | WRITEFILE | READFILE))
	    printf(" filename:%s", e->file_info.filename);
    printf(" exit_code: %u\n", e->process_info.exit_code);
    return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    return vfprintf(stderr, format, args);
}

int main()
{
    /* Set up libbpf errors and debug info callback */
    libbpf_set_print(libbpf_print_fn);
    StartProcesseBPFProg(handle_event, 4 * 1024 * 1024, 0xffffffff);
}
