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
 * Create: 2023-11-16
 * Description: ebpf converter
 */

#include "ebpf/ebpf_types.h"
#include "secDetector_topic.h"
#include <cstring>
#include <iostream>
#include <limits.h>
#include <map>
#include <sstream>
#include <unistd.h>

typedef std::string (*convert_func_t)(struct ebpf_event *e);

static std::string FindProcessPathFromPid(struct ebpf_event *e)
{
    char *path = NULL;
    std::string link = "/proc/" + std::to_string(e->pid) + "/exe";
    std::string exe = "null";

    path = new char[PATH_MAX];
    memset(path, 0, PATH_MAX);
    ssize_t len = readlink(link.c_str(), path, PATH_MAX);
    if (len != -1)
        exe = std::string(path);
    else
        exe = e->comm;
    delete path;
    return exe;
}

static std::string extract_common_info(struct ebpf_event *e)
{
    std::ostringstream ss;
    ss << "timestamp:" << e->timestamp << " event_name:" << e->event_name << " exe:" << FindProcessPathFromPid(e)
       << " pid:" << e->pid << " tgid:" << e->tgid << " uid:" << e->uid << " gid:" << e->gid << " comm:" << e->comm
       << " sid:" << e->sid << " ppid:" << e->ppid << " pcomm:" << e->pcomm << " nodename:" << e->nodename
       << " pns:" << e->pns << " root_pns:" << e->pns;
    return ss.str();
}

static std::string extract_common_process_info(struct ebpf_event *e)
{
    std::ostringstream ss;
    char umask_str[32] = { 0 };

    snprintf(umask_str, sizeof(umask_str), "%#04o", e->process_info.umask);
    ss << " umask:" << umask_str << " tracer_pid:" <<  e->process_info.tracer_pid;

    /* read all namespace */
    ss << " cgroup_ns:" << e->process_info.cgroup_ns;
    ss << " ipc_ns:" << e->process_info.ipc_ns;
    ss << " mnt_ns:" << e->process_info.mnt_ns;
    ss << " user_ns:" << e->process_info.user_ns;
    ss << " uts_ns:" << e->process_info.uts_ns;
    ss << " time_ns:" << e->process_info.time_ns;

    return ss.str();
}


static std::string convert_set_process_attr(struct ebpf_event *e)
{
    std::ostringstream ss;
    ss << extract_common_info(e) <<  extract_common_process_info(e) <<
           " new_uid:" << e->process_info.new_uid << " new_gid:" << e->process_info.new_gid;
    return ss.str();
}

static std::string convert_creat_process(struct ebpf_event *e)
{
    std::ostringstream ss;

    ss << extract_common_info(e) << extract_common_process_info(e);
    if (e->process_info.have_bprm)
        ss << " bprm_file:" << e->process_info.bprm_file;

    return ss.str();
}

static std::string convert_destroy_process(struct ebpf_event *e)
{
    std::ostringstream ss;

    ss << extract_common_info(e) << extract_common_process_info(e) << " exit_code:" << e->process_info.exit_code;
    return ss.str();
}

static std::string convert_common_file(struct ebpf_event *e)
{
    return extract_common_info(e) + " filename:" + e->file_info.filename ;
}

static std::string convert_set_file_attr(struct ebpf_event *e)
{
    return extract_common_info(e) + " filename:" + e->file_info.filename + " name:" + e->file_info.name;
}

static std::map<int, convert_func_t> convert_funcs = {
    {CREATPROCESS, convert_creat_process},
    {DESTROYPROCESS, convert_destroy_process},
    {SETPROCESSATTR, convert_set_process_attr},
    {CREATFILE, convert_common_file},
    {DELFILE, convert_common_file},
    {SETFILEATTR, convert_set_file_attr},
    {WRITEFILE, convert_common_file},
    {READFILE, convert_common_file},
};

std::string ebpf_event_to_text(struct ebpf_event *e)
{
    if (!e)
        return "";
    if (convert_funcs.count(e->type) == 0)
        return "undefined type for ebpf_event";
    return convert_funcs[e->type](e);
}
