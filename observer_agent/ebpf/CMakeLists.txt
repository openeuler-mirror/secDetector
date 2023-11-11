# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ebpf)
add_custom_target(ebpf ALL
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/.output
	COMMAND bpftool btf dump file /sys/kernel/btf/vmlinux format c > ${CMAKE_CURRENT_BINARY_DIR}/.output/vmlinux.h
	COMMAND clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -c ${CMAKE_CURRENT_SOURCE_DIR}/fentry.bpf.c -o ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.bpf.o
	COMMAND bpftool gen skeleton ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.bpf.o > ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.skel.h
	COMMAND cc -g -Wall -I${CMAKE_CURRENT_BINARY_DIR}/.output -I${CMAKE_CURRENT_SOURCE_DIR} -c ${CMAKE_CURRENT_SOURCE_DIR}/fentry.c -o ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.o
	COMMAND cc -g -Wall ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.o /usr/lib64/libbpf.a   -lelf -lz -o fentry
)


