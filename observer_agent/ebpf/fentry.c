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
 * Create: 2023-11-15
 * Description: secDetector process hook
 */
#include "fentry.h"
#include "secDetector_topic.h"
#include "ebpf_types.h"
#include "fentry.skel.h"
#include <bpf/libbpf.h>
#include <errno.h>
#include <linux/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

static volatile sig_atomic_t exiting;

void StopProcesseBPFProg()
{
    exiting = 1;
}

static void DisableProg(struct bpf_object_skeleton *s, const char *prog_name)
{
    int n = s->prog_cnt;

    for (int i = 0; i < n; i++) {
        if (strcmp(s->progs[i].name, prog_name) == 0) {
                fprintf(stderr, "%s is not enabled\n", prog_name);
                bpf_link__destroy(*s->progs[i].link);
                s->progs[i].link = NULL;

                /* exchange the last one */
                s->progs[i].prog = s->progs[n - 1].prog;
                s->progs[i].link = s->progs[n - 1].link;
                s->progs[i].name = s->progs[n - 1].name;
                s->prog_cnt--;
        }
    }
}


static void DisableProgBasedOnMask(struct bpf_object_skeleton *skel, int mask)
{
    if ((mask & CREATPROCESS) == 0) {
        DisableProg(skel, "handle_exec");
        DisableProg(skel, "handle_fork");
        DisableProg(skel, "handle_bprm_check");
    }

    if ((mask & DESTROYPROCESS) == 0) {
        DisableProg(skel, "handle_exit");
    }

    if ((mask & SETPROCESSATTR) == 0) {
        DisableProg(skel, "handle_commit_creds");
    }

    if ((mask & EXECCMD) == 0) {
        DisableProg(skel, "handle_execve_cmd");
    }
}


int StartProcesseBPFProg(ring_buffer_sample_fn cb, unsigned int rb_sz, int mask)
{
    struct fentry_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;

    /* Open load and verify BPF application */
    skel = fentry_bpf__open();
    if (!skel)
    {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }
    bpf_map__set_max_entries(skel->maps.rb, rb_sz);
    skel->rodata->secdetector_pid = getpid();

	if (fentry_bpf__load(skel)) {
        fprintf(stderr, "Failed to load BPF skeleton\n");
        err = -1;
        goto cleanup;
    }

    /* Attach tracepoint handler */
    err = fentry_bpf__attach(skel);
    if (err)
    {
        fprintf(stderr, "Failed to attach BPF skeleton\n");
        goto cleanup;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), cb, NULL, NULL);
    if (!rb)
    {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    DisableProgBasedOnMask(skel->skeleton, mask);

    while (exiting == 0)
    {
        err = ring_buffer__poll(rb, 1000 /* timeout ms*/);
        if (err == -EINTR)
        {
            err = 0;
            break;
        }
        if (err < 0)
        {
            fprintf(stderr, "poll failed, r:%d\n", err);
            break;
        }
    }

cleanup:
    ring_buffer__free(rb);
    fentry_bpf__destroy(skel);
    return -err;
}
