# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ebpf)
add_custom_target(ebpf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/.output
	COMMAND bpftool btf dump file /sys/kernel/btf/vmlinux format c > ${CMAKE_CURRENT_BINARY_DIR}/.output/vmlinux.h
	COMMAND clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_CURRENT_BINARY_DIR}/.output -c ${CMAKE_CURRENT_SOURCE_DIR}/fentry.bpf.c -o ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.bpf.o
	COMMAND bpftool gen skeleton ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.bpf.o > ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.skel.h
	COMMAND cc -g -Wall -fPIC -I${CMAKE_CURRENT_BINARY_DIR}/.output -I${CMAKE_CURRENT_SOURCE_DIR} -c ${CMAKE_CURRENT_SOURCE_DIR}/fentry.c -o ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.o
)

add_executable(test_fentry ${CMAKE_CURRENT_SOURCE_DIR}/test_fentry.c)
add_dependencies(test_fentry ebpf)
target_link_libraries(test_fentry ${CMAKE_CURRENT_BINARY_DIR}/.output/fentry.o z elf bpf)
