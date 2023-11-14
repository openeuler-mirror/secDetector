// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2021 Sartura
 * Based on minimal.c by Facebook */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <linux/types.h>
#include <bpf/libbpf.h>
#include "fentry.skel.h"
#include "fentry.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	return vfprintf(stderr, format, args);
}

static volatile sig_atomic_t stop;

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	const struct ebpf_event *e = data;
	printf("timestamp:%llu event_name:%s exe:%s pid:%u tgid:%u uid:%u gid:%u comm:%s"
	       " sid:%u ppid:%u pgid:%u pcomm:%s nodename:%s pns:%u root_pns:%u",
		       e->timestamp, e->event_name, e->exe, e->pid, e->tgid, e->uid, e->gid, e->comm, 
		       e->sid, e->ppid, e->pgid, e->pcomm, e->nodename, e->pns, e->root_pns);
	printf(" exit_code: %u\n", e->process_info.exit_code);
	return 0;
}

void stop_ebpf_prog()
{
	stop = 1;
}

int start_and_poll_ebpf_prog()
{
	struct fentry_bpf *skel;
	struct ring_buffer *rb = NULL;
	int err;

	/* Set up libbpf errors and debug info callback */
	libbpf_set_print(libbpf_print_fn);

	/* Open load and verify BPF application */
	skel = fentry_bpf__open_and_load();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	/* Attach tracepoint handler */
	err = fentry_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
	if (!rb) {
		err = -1;
		fprintf(stderr, "Failed to create ring buffer\n");
		goto cleanup;
	}

	while (!stop) {
		err = ring_buffer__poll(rb, -1);
		if (err == -EINTR) {
			err = 0;
			break;
		}
		if (err < 0) {
			fprintf(stderr, "poll failed, r:%d\n", err);
			break;
		}
	}

cleanup:
	ring_buffer__free(rb);
	fentry_bpf__destroy(skel);
	return -err;
}
