#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include "resources_collect.h"
#include "json11.hh"
#include <thread>

std::vector<ProcessInfo> getProcesses() {
    std::vector<ProcessInfo> processes;

    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        std::cerr << "Failed open /proc 。" << std::endl;
        return processes;
    }

    dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string name = entry->d_name;
            if (name.find_first_not_of("0123456789") == std::string::npos) {
                pid_t pid = std::stoi(name);
                std::string statFile = "/proc/" + name + "/stat";

                std::ifstream statFileStream(statFile);
                if (statFileStream.is_open()) {
                    std::string statLine;
                    std::getline(statFileStream, statLine);
                    statFileStream.close();

                    std::istringstream iss(statLine);
                    std::string processName;
                    iss >> processName; 

                    ProcessInfo process;
                    process.pid = pid;
                    process.name = processName;

                    processes.push_back(process);
                }
            }
        }
    }

    closedir(dir);

    return processes;
}

std::vector<pid_t> getThreads(pid_t pid) {
    std::vector<pid_t> threads;

    DIR* dir = opendir(("/proc/" + std::to_string(pid) + "/task").c_str());
    if (dir == nullptr) {
        std::cerr << "Failed open /proc/" << pid << "/task。" << std::endl;
        return threads;
    }

    dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string name = entry->d_name;
            if (name.find_first_not_of("0123456789") == std::string::npos) {
                pid_t tid = std::stoi(name);
                threads.push_back(tid);
            }
        }
    }

    closedir(dir);

    return threads;
}

std::vector<pid_t> getChildProcesses(pid_t pid) {
    std::vector<pid_t> childProcesses;

    DIR* dir = opendir(("/proc/" + std::to_string(pid) + "/task").c_str());
    if (dir == nullptr) {
        std::cerr << "Failed open /proc/" << pid << "/task。" << std::endl;
        return childProcesses;
    }

    dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string name = entry->d_name;
            if (name.find_first_not_of("0123456789") == std::string::npos) {
                pid_t tid = std::stoi(name);
                if (tid != pid) {  
                    childProcesses.push_back(tid);
                }
            }
        }
    }

    closedir(dir);

    return childProcesses;
}
json11::Json getProcessesInfoJson(int totalProcessCount, int totalThreadCount, int totalChildProcesses, double pidUsageRatio, int pid, int threadsSize, int childProcessesSize)
{

    // 将 PID 使用率转换为字符串
    std::ostringstream pidUsageStr;
    pidUsageStr << std::fixed << pidUsageRatio;

    return json11::Json::object{
        {"Total_Process", totalProcessCount},
        {"Total_Threads", totalThreadCount},
        {"Total_Child_Processes", totalChildProcesses},
        {"PID_Usage_Ratio", pidUsageStr.str()},
        {"PID", pid},
        {"Child_Processes", threadsSize},
        {"Child_Threads", childProcessesSize}};
}


json11::Json getProcessesInfoAll()
{
    std::vector<ProcessInfo> processes = getProcesses();

    int totalProcessCount = processes.size();
    int totalThreadCount = 0;
    int totalChildProcesses = 0;

    for (const ProcessInfo &process : processes)
    {
        std::vector<pid_t> threads = getThreads(process.pid);
        std::vector<pid_t> childProcesses = getChildProcesses(process.pid);
        totalThreadCount += threads.size();
        totalChildProcesses += childProcesses.size();
    }

    // Calculate PID usage ratio
    std::ifstream pidMaxFile("/proc/sys/kernel/pid_max");
    int pidMax = 0;
    if (pidMaxFile.is_open())
    {
        pidMaxFile >> pidMax;
        pidMaxFile.close();
    }

    double pidUsageRatio = static_cast<double>(totalProcessCount) / pidMax;

    json11::Json processInfoJson = getProcessesInfoJson(totalProcessCount, totalThreadCount, totalChildProcesses, pidUsageRatio, 0, 0, 0);
    return processInfoJson;
}

json11::Json getProcessesInfoByPid(pid_t pid)
{
    std::vector<ProcessInfo> processes = getProcesses();

    int totalProcessCount = processes.size();
    int totalThreadCount = 0;
    int totalChildProcesses = 0;

    for (const ProcessInfo &process : processes)
    {
        std::vector<pid_t> threads = getThreads(process.pid);
        std::vector<pid_t> childProcesses = getChildProcesses(process.pid);
        totalThreadCount += threads.size();
        totalChildProcesses += childProcesses.size();
    }

    // Calculate PID usage ratio
    std::ifstream pidMaxFile("/proc/sys/kernel/pid_max");
    int pidMax = 0;
    if (pidMaxFile.is_open())
    {
        pidMaxFile >> pidMax;
        pidMaxFile.close();
    }

    double pidUsageRatio = static_cast<double>(totalProcessCount) / pidMax;

    std::vector<pid_t> threads = getThreads(pid);
    std::vector<pid_t> childProcesses = getChildProcesses(pid);

    json11::Json processInfoJson = getProcessesInfoJson(totalProcessCount, totalThreadCount, totalChildProcesses, pidUsageRatio, pid, threads.size(), childProcesses.size());
    return processInfoJson;
}


