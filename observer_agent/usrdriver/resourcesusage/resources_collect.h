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
 * Author: zengwei
 * Create: 2023-10-24
 * Description: secDetector resource collection probe
 */

#include "json11.hh"
#include <string>
#include <vector>

using namespace std;

struct MemoryInfo
{
    unsigned long totalMemory;
    unsigned long usedMemory;
    unsigned long freeMemory;
    double usagePercentage;
    double freePercentage;
    bool warningFlag;
    string timestamp;
};

struct FileSystemInfo
{
    std::string mountPoint;
    std::string totalInodes;
    std::string usedInodes;
    std::string inodeUsageRatio;
};

struct ProcessInfo
{
    pid_t pid;
    std::string name;
    std::vector<pid_t> threads;
    std::vector<pid_t> childProcesses;
};

struct DiskUsage
{
    string filesystem;
    string size;
    string used;
    string available;
    string usePercentage;
    string mountPoint;
};

MemoryInfo getMemoryInfo();
string getTimeStamp();
json11::Json getAllMemoryInfo();
json11::Json getDiskUsageString();
json11::Json updateCpuUsage();
json11::Json getInfofileJson();
json11::Json getInfofileJsonPID(int pid);
json11::Json getProcessesInfoAll();
json11::Json getProcessesInfoByPid(pid_t pid);
json11::Json tcp4data();