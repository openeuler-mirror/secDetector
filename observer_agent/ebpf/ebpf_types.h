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
 * Description: ebpf event types
 */
#ifndef __SECDETECTOR_EBPF_TYPES_H
#define __SECDETECTOR_EBPF_TYPES_H
#ifdef __cplusplus
extern "C"
{
#endif

#define TASK_COMM_SIZE 16
#define EVENT_NAME_SIZE 32
#define MAX_FILENAME_SIZE 256
#define MAX_TEXT_SIZE 4096

struct process_info
{
    unsigned exit_code;
    struct
    {
        unsigned new_uid;
        unsigned new_gid;
    };
    struct
    {
        char bprm_file[MAX_FILENAME_SIZE];
        int have_bprm;
    };
};

struct file_info
{
    char filename[MAX_TEXT_SIZE];
    char name[MAX_TEXT_SIZE];
    char value[MAX_TEXT_SIZE];
    char old_value[MAX_TEXT_SIZE];
};

struct ebpf_event
{
    int type;
    char event_name[EVENT_NAME_SIZE];
    // common info
    unsigned long long timestamp;
    unsigned uid;
    unsigned gid;
    char exe[MAX_FILENAME_SIZE];
    unsigned pid;
    char comm[TASK_COMM_SIZE];
    unsigned tgid;
    unsigned pgid;
    unsigned ppid;
    char pcomm[TASK_COMM_SIZE];
    unsigned sid;
    char nodename[MAX_FILENAME_SIZE];
    unsigned pns;
    unsigned root_pns;
    union {
        struct process_info process_info;
        struct file_info file_info;
    };
};

#ifdef __cplusplus
}
#endif
#endif
