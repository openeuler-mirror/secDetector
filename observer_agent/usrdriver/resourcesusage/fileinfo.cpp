#include "json11.hh"
#include "resources_collect.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

int getMaxSystemFileDescriptors()
{
    std::ifstream file("/proc/sys/fs/file-max");
    if (file.is_open())
    {
        int maxFileDescriptors;
        file >> maxFileDescriptors;
        file.close();
        return maxFileDescriptors;
    }
    return -1;
}

int getSystemOpenFileDescriptors()
{
    std::ifstream file("/proc/sys/fs/file-nr");
    if (file.is_open())
    {
        int openFileDescriptors;
        file >> openFileDescriptors;
        file.close();
        return openFileDescriptors;
    }
    return -1;
}

int getMaxProcessFileDescriptors()
{
    std::ifstream file("/proc/sys/fs/nr_open");
    if (file.is_open())
    {
        int maxProcessFileDescriptors;
        file >> maxProcessFileDescriptors;
        file.close();
        return maxProcessFileDescriptors;
    }
    return -1;
}

int getProcessOpenFileCount(int pid)
{
    std::string command = "ls -l /proc/" + std::to_string(pid) + "/fd | wc -l";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }

    char buffer[128];
    std::string result = "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != nullptr)
            result += buffer;
    }
    pclose(pipe);

    return std::atol(result.c_str());
}

int getMaxProcessFiles(int pid)
{
    std::ifstream file("/proc/" + std::to_string(pid) + "/limits");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("Max open files") != std::string::npos)
            {
                std::istringstream iss(line);
                std::string dummy;
                int maxProcessFiles;
                iss >> dummy >> dummy >> dummy >> maxProcessFiles;
                file.close();
                return maxProcessFiles;
            }
        }
        file.close();
    }
    return -1;
}

std::vector<FileSystemInfo> getAllFileSystemInfo()
{
    std::vector<FileSystemInfo> filesystems;

    std::string command = "df -i";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "popen failed!" << std::endl;
        return filesystems;
    }

    char buffer[256];
    bool headerSkipped = false;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::istringstream iss(buffer);

        std::string filesystem, inodes, used, available, usageRatio, mountPoint;
        if (iss >> filesystem >> inodes >> used >> available >> usageRatio >> mountPoint)
        {
            if (!headerSkipped)
            {
                headerSkipped = true;
            }
            else
            {
                FileSystemInfo info;
                info.mountPoint = mountPoint;
                info.totalInodes = inodes;
                info.usedInodes = used;
                info.inodeUsageRatio = usageRatio;

                filesystems.push_back(info);
            }
        }
    }

    pclose(pipe);

    return filesystems;
}

json11::Json getFileUsageJson(int maxSystemFileDescriptors, int systemOpenFileDescriptors,
                              int maxProcessFileDescriptors, int processOpenFileCount, int maxProcessFiles)
{
    json11::Json::object fileJson{
        {"System_Max_File_Descriptors", maxSystemFileDescriptors},
        {"Process_Max_File_Descriptors", maxProcessFileDescriptors},
        {"Process_Open_File_Descriptors", processOpenFileCount},
        {"Process_Max_Open_File_Descriptors", maxProcessFiles},
        {"System_Open_File_Descriptors", systemOpenFileDescriptors},
    };

    return fileJson;
}


json11::Json getInfofileJsonPID(int pid)
{
    int maxSystemFileDescriptors = getMaxSystemFileDescriptors();
    int systemOpenFileDescriptors = getSystemOpenFileDescriptors();
    int maxProcessFileDescriptors = getMaxProcessFileDescriptors();
    int processOpenFileCount = getProcessOpenFileCount(pid);
    int maxProcessFiles = getMaxProcessFiles(pid);

    std::vector<FileSystemInfo> filesystems = getAllFileSystemInfo();

    json11::Json::object jsonObject{
        {"FileUsage", getFileUsageJson(maxSystemFileDescriptors, systemOpenFileDescriptors,
                                       maxProcessFileDescriptors, processOpenFileCount, maxProcessFiles)}
    };

    json11::Json::array filesystemArray;
    for (const auto &fs : filesystems)
    {
        json11::Json::object fsObject{
            {"MountPoint", fs.mountPoint},
            {"TotalInodes", fs.totalInodes},
            {"UsedInodes", fs.usedInodes},
            {"InodeUsageRatio", fs.inodeUsageRatio},
        };
        filesystemArray.push_back(fsObject);
    }
    jsonObject["FileSystems"] = filesystemArray;

    return jsonObject;
}

json11::Json getInfofileJson()
{
    int maxSystemFileDescriptors = getMaxSystemFileDescriptors();
    int systemOpenFileDescriptors = getSystemOpenFileDescriptors();
    int maxProcessFileDescriptors = getMaxProcessFileDescriptors();

    std::vector<FileSystemInfo> filesystems = getAllFileSystemInfo();

    json11::Json::object jsonObject{
        {"FileUsage", getFileUsageJson(maxSystemFileDescriptors, systemOpenFileDescriptors,
                                       maxProcessFileDescriptors, 0, 0)}
    };

    json11::Json::array filesystemArray;
    for (const auto &fs : filesystems)
    {
        json11::Json::object fsObject{
            {"MountPoint", fs.mountPoint},
            {"TotalInodes", fs.totalInodes},
            {"UsedInodes", fs.usedInodes},
            {"InodeUsageRatio", fs.inodeUsageRatio},
        };
        filesystemArray.push_back(fsObject);
    }
    jsonObject["FileSystems"] = filesystemArray;

    return jsonObject;
}
