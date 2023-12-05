#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <chrono>
#include "resources_collect.h"
#include "json11.hh"

using namespace std;

vector<DiskUsage> getDiskUsageList()
{
    vector<DiskUsage> diskUsageList;

    FILE *pipe = popen("df -h", "r");
    if (pipe)
    {
        char buffer[256];
        fgets(buffer, sizeof(buffer), pipe);

        while (!feof(pipe))
        {
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                istringstream iss(buffer);
                DiskUsage diskUsage;
                iss >> diskUsage.filesystem >> diskUsage.size >> diskUsage.used >> diskUsage.available >> diskUsage.usePercentage >> diskUsage.mountPoint;
                diskUsageList.push_back(diskUsage);
            }
        }
        pclose(pipe);
    }

    return diskUsageList;
}

json11::Json getDiskUsageString()
{
    static vector<DiskUsage> previousDiskUsageList;

    vector<DiskUsage> diskUsageList = getDiskUsageList();

    json11::Json::array diskUsageArray;

    double totalUsedSpace = 0.0;
    double totalAvailableSpace = 0.0;

    for (size_t i = 0; i < diskUsageList.size(); ++i)
    {
        const DiskUsage &diskUsage = diskUsageList[i];

        json11::Json::object diskUsageObject;
        diskUsageObject["Filesystem"] = diskUsage.filesystem;
        diskUsageObject["Size"] = diskUsage.size;
        diskUsageObject["Used"] = diskUsage.used;
        diskUsageObject["Available"] = diskUsage.available;
        diskUsageObject["UsePercentage"] = diskUsage.usePercentage;
        diskUsageObject["MountPoint"] = diskUsage.mountPoint;

        if (i < previousDiskUsageList.size())
        {
            const DiskUsage &previousDiskUsage = previousDiskUsageList[i];
            double currentUsedSpace = 0.0;
            double previousUsedSpace = 0.0;

            size_t currentUsedSizePos = diskUsage.used.find('G');
            size_t previousUsedSizePos = previousDiskUsage.used.find('G');
            if (currentUsedSizePos != string::npos && previousUsedSizePos != string::npos)
            {
                currentUsedSpace = stod(diskUsage.used.substr(0, currentUsedSizePos));
                previousUsedSpace = stod(previousDiskUsage.used.substr(0, previousUsedSizePos));
            }

            double usedSpaceDiff = currentUsedSpace - previousUsedSpace;
            diskUsageObject["UsedSpaceDifference"] = usedSpaceDiff;
        }

        size_t usedSizePos = diskUsage.used.find('G');
        if (usedSizePos != string::npos)
        {
            double usedSpace = stod(diskUsage.used.substr(0, usedSizePos));
            totalUsedSpace += usedSpace;
        }

        size_t availableSizePos = diskUsage.available.find('G');
        if (availableSizePos != string::npos)
        {
            double availableSpace = stod(diskUsage.available.substr(0, availableSizePos));
            totalAvailableSpace += availableSpace;
        }

        diskUsageArray.push_back(std::move(diskUsageObject));
    }

    json11::Json::object resultObject;
    resultObject["DiskUsageList"] = std::move(diskUsageArray);
    resultObject["TotalUsedSpace"] = totalUsedSpace;
    resultObject["TotalAvailableSpace"] = totalAvailableSpace;



    previousDiskUsageList = diskUsageList;

    return resultObject;
}
