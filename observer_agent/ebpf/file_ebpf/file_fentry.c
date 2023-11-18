// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2021 Sartura
 * Based on minimal.c by Facebook */

#include "../ebpf_types.h"
#include "../fentry.h"
#include "file_fentry.skel.h"
#include "secDetector_topic.h"
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
                break;
        }
    }
}

static void DisableProgBasedOnMask(struct bpf_object_skeleton *skel, int mask)
{
    if ((mask & CREATFILE) == 0) {
        DisableProg(skel, "do_filp_open_exit");
    }

    if ((mask & DELFILE) == 0) {
        DisableProg(skel, "fexit_vfs_unlink");
    }

    if ((mask & WRITEFILE) == 0) {
        DisableProg(skel, "fexit_vfs_write");
    }

    if ((mask & READFILE) == 0) {
        DisableProg(skel, "fexit_vfs_read");
    }

    if ((mask & SETFILEATTR) == 0) {
        DisableProg(skel, "fexit_vfs_utimes");
        DisableProg(skel, "fexit_chown_common");
        DisableProg(skel, "fexit_chmod_common");
        DisableProg(skel, "fentry__vfs_setxattr_noperm");
        DisableProg(skel, "fentry__vfs_removexattr_locked");
        DisableProg(skel, "fentry_vfs_rename");
    }
}

int StartFileBPFProg(ring_buffer_sample_fn cb, unsigned int rb_sz, int mask)
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
    file_fentry_bpf__destroy(skel);
    return -err;
}
