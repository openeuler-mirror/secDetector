#include "json11.hh"
#include "resources_collect.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using namespace std;

bool readCpuStat(unsigned long long &user, unsigned long long &nice, unsigned long long &system,
                 unsigned long long &idle)
{
    ifstream statFile("/proc/stat");
    string line;

    while (getline(statFile, line))
    {
        istringstream iss(line);
        string cpuLabel;
        iss >> cpuLabel;

        if (cpuLabel == "cpu")
        {
            iss >> user >> nice >> system >> idle;
            return true;
        }
    }

    return false;
}

unsigned long long getTotalTime(unsigned long long user, unsigned long long nice, unsigned long long system,
                                unsigned long long idle)
{
    return user + nice + system + idle;
}

double calculateCpuUsage(unsigned long long prevTotalTime, unsigned long long currTotalTime,
                         unsigned long long prevIdleTime, unsigned long long currIdleTime)
{
    unsigned long long totalTimeDiff = currTotalTime - prevTotalTime;
    unsigned long long idleTimeDiff = currIdleTime - prevIdleTime;

    if (totalTimeDiff == 0)
    {
        return 0.0;
    }

    double cpuUsage = 100.0 * (totalTimeDiff - idleTimeDiff) / totalTimeDiff;
    return cpuUsage;
}

json11::Json getCpuUsageJson(double cpuUsage, double cpuUsageDiff)
{
    return json11::Json::object{{"CPU_Usage", cpuUsage}, {"Diff", cpuUsageDiff}};
}


json11::Json updateCpuUsage()
{
    static unsigned long long prevUser = 0, prevNice = 0, prevSystem = 0, prevIdle = 0;
    static double prevCpuUsage = 0.0;

    unsigned long long currUser, currNice, currSystem, currIdle;

    if (!readCpuStat(currUser, currNice, currSystem, currIdle))
    {
        cerr << "无法读取/proc/stat" << endl;
        return json11::Json::object {};
    }

    unsigned long long prevTotal = getTotalTime(prevUser, prevNice, prevSystem, prevIdle);
    unsigned long long prevIdleTime = prevIdle;

    unsigned long long currTotal = getTotalTime(currUser, currNice, currSystem, currIdle);
    unsigned long long currIdleTime = currIdle;

    double currCpuUsage = calculateCpuUsage(prevTotal, currTotal, prevIdleTime, currIdleTime);

    double cpuUsageDiff = currCpuUsage - prevCpuUsage;
    if (prevCpuUsage == 0)
    {
        cpuUsageDiff = 0;
    }

    prevUser = currUser;
    prevNice = currNice;
    prevSystem = currSystem;
    prevIdle = currIdle;
    prevCpuUsage = currCpuUsage;

    json11::Json cpuUsageJson = getCpuUsageJson(currCpuUsage, cpuUsageDiff);
    return cpuUsageJson;
}

