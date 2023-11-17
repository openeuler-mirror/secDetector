// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2021 Sartura
 * Based on minimal.c by Facebook */

#include "../ebpf_types.h"
#include "../fentry.h"
#include "file_fentry.skel.h"
#include <bpf/libbpf.h>
#include <errno.h>
#include <linux/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

static volatile sig_atomic_t exiting;

void StopFileBPFProg()
{
    exiting = 1;
}

int StartFileBPFProg(ring_buffer_sample_fn cb, unsigned int rb_sz)
{
    struct file_fentry_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;

    /* Open load and verify BPF application */
    skel = file_fentry_bpf__open();
    if (!skel)
    {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }
    bpf_map__set_max_entries(skel->maps.rb, rb_sz);
    skel->rodata->secdetector_pid = getpid();

	if (file_fentry_bpf__load(skel)) {
        fprintf(stderr, "Failed to load BPF skeleton\n");
        err = -1;
        goto cleanup;
    }

    /* Attach tracepoint handler */
    err = file_fentry_bpf__attach(skel);
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
    file_fentry_bpf__destroy(skel);
    return -err;
}
