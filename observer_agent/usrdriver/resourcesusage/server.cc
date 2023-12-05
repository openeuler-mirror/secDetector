#include "json11.hh"
#include <iostream>
#include "resources_collect.h"


json11::Json get_data(){

    json11::Json json = json11::Json::object {
        {"tcp", tcp4data()},
        {"cpuinfo", updateCpuUsage()},
        {"diskinfo", getDiskUsageString()},
        {"fileinfo", getInfofileJson()},
        {"meminfo", getAllMemoryInfo()},
        {"pidinfo", getProcessesInfoAll()},

    };
    
    return json;
}

