/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Author: chenjingwen
 * Create: 2023-09-25
 * Description: secDetector ringbuffer header
 */
#ifndef SECDETECTOR_RINGBUFFER_H
extern int secDetector_ringbuf_output(void *data, u64 size, u64 flags);
extern int __init secDetector_ringbuf_dev_init(void);
extern void __exit secDetector_ringbuf_dev_exit(void);

#define SECDETECTOR_RINGBUFFER_H
#endif
