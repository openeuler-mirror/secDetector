#include "json11.hh"
#include "resources_collect.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

string getTimeStamp()
{
    time_t currentTime = time(nullptr);
    tm *timeInfo = localtime(&currentTime);
    ostringstream oss;
    oss << put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

MemoryInfo getMemoryInfo()
{
    MemoryInfo memInfo;
    ifstream meminfoFile("/proc/meminfo");

    if (!meminfoFile.is_open())
    {
        cout << "Failed to open /proc/meminfo." << endl;
        memInfo.totalMemory = -1;
        memInfo.usedMemory = -1;
        memInfo.freeMemory = -1;
        memInfo.usagePercentage = -1.0;
        memInfo.freePercentage = -1.0;
        memInfo.warningFlag = false;
        return memInfo;
    }

    string line;
    while (getline(meminfoFile, line))
    {
        istringstream iss(line);
        string key;
        unsigned long long value;
        if (iss >> key >> value)
        {
            if (key == "MemTotal:")
            {
                memInfo.totalMemory = value;
            }
            else if (key == "MemAvailable:")
            {
                memInfo.freeMemory = value;
            }
        }
    }

    memInfo.usedMemory = memInfo.totalMemory - memInfo.freeMemory;
    memInfo.usagePercentage = (static_cast<double>(memInfo.usedMemory) / memInfo.totalMemory) * 100.0;
    memInfo.freePercentage = (static_cast<double>(memInfo.freeMemory) / memInfo.totalMemory) * 100.0;
    memInfo.timestamp = time(nullptr);
    memInfo.warningFlag = (memInfo.usagePercentage > 90.0);

    return memInfo;
}

json11::Json createJsonFromMemoryInfo(int UsedMemoryDifference, double UsagePercentageDifference,
                                      unsigned long TotalMemory, unsigned long UsedMemory, unsigned long FreeMemory,
                                      double UsagePercentage, double FreePercentage)
{
    return json11::Json::object{{"UsedMemoryDifference", UsedMemoryDifference},
                                {"UsagePercentageDifference", UsagePercentageDifference},
                                {"TotalMemory", static_cast<double>(TotalMemory)},
                                {"UsedMemory", static_cast<double>(UsedMemory)},
                                {"FreeMemory", static_cast<double>(FreeMemory)},
                                {"UsagePercentage", UsagePercentage},
                                {"FreePercentage", FreePercentage}};
}


json11::Json getAllMemoryInfo()
{
    static MemoryInfo prevMemInfo;

    MemoryInfo currentMemInfo = getMemoryInfo();

    int usedMemoryDiff = currentMemInfo.usedMemory - prevMemInfo.usedMemory;
    double usagePercentageDiff = currentMemInfo.usagePercentage - prevMemInfo.usagePercentage;

    if (prevMemInfo.usedMemory == 0)
    {
        usedMemoryDiff = 0;
        usagePercentageDiff = 0;
    }

    prevMemInfo = currentMemInfo;

    prevMemInfo.timestamp = getTimeStamp();

    json11::Json memoryInfoJson =
        createJsonFromMemoryInfo(usedMemoryDiff, usagePercentageDiff, prevMemInfo.totalMemory, prevMemInfo.usedMemory,
                                 prevMemInfo.freeMemory, prevMemInfo.usagePercentage, prevMemInfo.freePercentage);
    return memoryInfoJson;
}