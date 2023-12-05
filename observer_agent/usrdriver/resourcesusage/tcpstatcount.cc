#include <fstream>
#include <sstream>
#include <string>
#include <functional>
#include <iostream>

#include "resources_collect.h"

#define debug_info 1

json11::Json tcp4statcount() {
    json11::Json emptyjson = json11::Json::object {};
    std::ifstream tcpFile("/proc/net/tcp");
    if(!tcpFile.is_open()){
        std::cerr << "Failed to open /proc/net/tcp" << std::endl;
        return emptyjson;
    }
    

    std::string line;
    int tcpConnectionCount {0};         // TCP连接总数
    int halfOpenQueueOccupied {0};      // 半连接队列已用
    int halfOpenQueueCapacity {0};      // 半连接队列容量
    int fullOpenQueueOccupied {0};      // 全连接队列已用
    int fullOpenQueueCapacity {0};      // 全连接队列容量
    int establishedCount {0};           // ESTABLISHED状态连接数
    int synSentCount {0};               // SYN_SENT状态连接数
    int synRecvCount {0};               // SYN_RECV状态连接数
    int finWait1Count {0};              // FIN_WAIT1状态连接数
    int finWait2Count {0};              // FIN_WAIT2状态连接数
    int timeWaitCount {0};              // TIME_WAIT状态连接数
    int closeCount {0};                 // CLOSE状态连接数
    int closeWaitCount {0};             // CLOSE_WAIT状态连接数
    int lastAckCount {0};               // LAST_ACK状态连接数
    int closingCount {0};               // CLOSING状态连接数
    // AL: 5pWw5o2u57G75Z6L5bqU5b2T5LyY5YyW

    // 解析TCP数据
    while (std::getline(tcpFile, line)) {
        // 跳过标题行
        if (line.find("sl") != std::string::npos) {
            continue;
        }

        tcpConnectionCount++;

        std::istringstream iss(line);
        std::string col;
        int colCount {0};

        // 解析每一列
        while (std::getline(iss, col, ' ')) {
            if (!col.empty()) {
                colCount++;
                // 第四列是连接状态
                if (colCount == 4) {
                    switch (std::stoi(col, nullptr, 16)) {
                        case 0x01:      establishedCount++;     break;      // ESTABLISHED
                        case 0x02:      synSentCount++;         break;      // SYN_SENT
                        case 0x03:      synRecvCount++;         break;      // SYN_RECV
                        case 0x04:      finWait1Count++;        break;      // FIN_WAIT1
                        case 0x05:      finWait2Count++;        break;      // FIN_WAIT2
                        case 0x06:      timeWaitCount++;        break;      // TIME_WAIT
                        case 0x07:      closeCount++;           break;      // CLOSE
                        case 0x08:      closeWaitCount++;       break;      // CLOSE_WAIT
                        case 0x09:      lastAckCount++;         break;      // LAST_ACK
                        case 0x0A:                              break;      // LISTEN 不计数
                        case 0x0B:      closingCount++;         break;      // CLOSING
                        default:                                break;
                    }
                }
            }
        }
    }

    // 计算半连接队列占用情况
    halfOpenQueueOccupied = synSentCount + synRecvCount;

    // 计算全连接队列占用情况
    fullOpenQueueOccupied = establishedCount;

    std::ifstream queueFile("/proc/sys/net/ipv4/tcp_max_syn_backlog");
    if(!queueFile.is_open()){
        std::cerr << "Failed to open /proc/sys/net/ipv4/tcp_max_syn_backlog" << std::endl;
        return emptyjson;
    }
    queueFile >> halfOpenQueueCapacity;
    std::ifstream fullQueueFile("/proc/sys/net/core/somaxconn");
    if(!fullQueueFile.is_open()){
        std::cerr << "Failed to open /proc/sys/net/core/somaxconn" << std::endl;
        return emptyjson;
    }
    fullQueueFile >> fullOpenQueueCapacity;


#if debug_info
    std::cout << "tcpConnectionCount: "         << tcpConnectionCount       << std::endl;
    std::cout << "halfOpenQueueOccupied: "      << halfOpenQueueOccupied    << std::endl;
    std::cout << "halfOpenQueueCapacity: "      << halfOpenQueueCapacity    << std::endl;
    std::cout << "halfconnqueueoccupancy: "     << (halfOpenQueueOccupied / (double)halfOpenQueueCapacity) * 100 << "%" << std::endl;
    std::cout << "fullOpenQueueOccupied: "      << fullOpenQueueOccupied    << std::endl;
    std::cout << "fullOpenQueueCapacity: "      << fullOpenQueueCapacity    << std::endl;
    std::cout << "fullconnqueueoccupancy: "     << (fullOpenQueueOccupied / (double)fullOpenQueueCapacity) * 100 << "%" << std::endl;
    std::cout << "establishedCount "            << establishedCount         << std::endl;
    std::cout << "closeCount: "                 << closeCount               << std::endl;
    std::cout << "timeWaitCount: "              << timeWaitCount            << std::endl;
#endif
    json11::Json::object tcp4count_res{
            {"tcpConnectionCount",      tcpConnectionCount},
            {"establishedCount",        establishedCount},
            {"synSentCount",            synSentCount},
            {"synRecvCount",            synRecvCount},
            {"finWait1Count",           finWait1Count},
            {"finWait2Count",           finWait2Count},
            {"timeWaitCount",           timeWaitCount},
            {"closeCount",              closeCount},
            {"closeWaitCount",          closeWaitCount},
            {"lastAckCount",            lastAckCount},
            {"closingCount",            closingCount},
            {"halfOpenQueueOccupied",   halfOpenQueueOccupied},
            {"halfOpenQueueCapacity",   halfOpenQueueCapacity},
            {"fullOpenQueueOccupied",   fullOpenQueueOccupied},
            {"fullOpenQueueCapacity",   fullOpenQueueCapacity}
        };
        return tcp4count_res;
}

 json11::Json tcp4buffer() {
    auto readFile = [](const std::string& filePath) -> std::tuple<int, int, int> {
        int result1 {0};
        int result2 {0};
        int result3 {0};
        try {
            std::ifstream file(filePath);
            if (file.is_open()) {
                file >> result1 >> result2 >> result3;
            } else {
                throw std::runtime_error("Failed to open file: " + filePath);
            }
        } catch ( std::exception& e) {
            std::cerr << "Exception occurred for file: " << filePath << ", Error: " << e.what() << std::endl;
        }
        return std::make_tuple(result1, result2, result3);
    };
    
    int tcpMem_low {0};
    int tcpMem_med {0};
    int tcpMem_hig {0};
    std::tie(tcpMem_low, tcpMem_med, tcpMem_hig) = readFile("/proc/sys/net/ipv4/tcp_mem");
    
    int tcpRmem_min {0};
    int tcpRmem_default {0};
    int tcpRmem_max {0};
    std::tie(tcpRmem_min, tcpRmem_default, tcpRmem_max) = readFile("/proc/sys/net/ipv4/tcp_rmem");

    int tcpWmem_min {0};
    int tcpWmem_default {0};
    int tcpWmem_max {0};
    std::tie(tcpWmem_min, tcpWmem_default, tcpWmem_max) = readFile("/proc/sys/net/ipv4/tcp_wmem");


    auto getTotalBufferUsage = [](const std::string& filePath) -> std::tuple<int, int> {
        std::ifstream file(filePath);
        std::string line;
        int totalTxQueue = 0, totalRxQueue = 0;

        // Skip the header line
        std::getline(file, line);

        while (std::getline(file, line)) {
            int txQueue, rxQueue;
            sscanf(line.c_str(), "%*d: %*s %*s %*s %x:%x %*x:%*x %*x %*d %*d %*d %*d %*d %*d %*d",
                &txQueue, &rxQueue);
            totalTxQueue += txQueue;
            totalRxQueue += rxQueue;
        }
        return std::make_tuple(totalTxQueue, totalRxQueue);
    };

    int totalTxQueue, totalRxQueue;
    std::tie(totalTxQueue, totalRxQueue) = getTotalBufferUsage("/proc/net/tcp");

    int tcp4stack_allmem {0};
    std::ifstream file("/proc/net/sockstat");
    std::string line;

    std::getline(file, line);
    std::getline(file, line);

    std::istringstream iss(line);
    std::string word;
    std::string lastNumber;
    while (iss >> word) {
        lastNumber = word;
    }
    tcp4stack_allmem = std::stoll(lastNumber);

#if debug_info
    std::cout << "tcpMem_low: "     << tcpMem_low       << std::endl;
    std::cout << "tcpMem_med: "     << tcpMem_med       << std::endl;
    std::cout << "tcpMem_hig: "     << tcpMem_hig       << std::endl;
    std::cout << "tcpRmem_min: "    << tcpRmem_min      << std::endl;
    std::cout << "tcpRmem_default: "<< tcpRmem_default  << std::endl;
    std::cout << "tcpRmem_max: "    << tcpRmem_max      << std::endl;
    std::cout << "tcpWmem_min: "    << tcpWmem_min      << std::endl;
    std::cout << "tcpWmem_default: "<< tcpWmem_default  << std::endl;
    std::cout << "tcpWmem_max: "    << tcpWmem_max      << std::endl;
    std::cout << "totalTxQueue: "   << totalTxQueue     << std::endl;
    std::cout << "totalRxQueue: "   << totalRxQueue     << std::endl;
    std::cout << "tcp4stack_allmem" << tcp4stack_allmem << std::endl;
#endif

    json11::Json::object tcp4buffer_res{
        {"tcpMem_low",      tcpMem_low},
        {"tcpMem_med",      tcpMem_med},
        {"tcpMem_hig",      tcpMem_hig},
        {"tcpRmem_min",     tcpRmem_min},
        {"tcpRmem_default", tcpRmem_default},
        {"tcpRmem_max",     tcpRmem_max},
        {"tcpWmem_min",     tcpWmem_min},
        {"tcpWmem_default", tcpWmem_default},
        {"tcpWmem_max",     tcpWmem_max},
        {"totalTxQueue",    totalTxQueue},
        {"totalRxQueue",    totalRxQueue},
        {"tcp4stack_allmem",tcp4stack_allmem}
    };
    return tcp4buffer_res;
}

json11::Json tcp4data() {
    auto statcount = tcp4statcount();
    // std::cout << statcount.dump() << std::endl;
    auto buffercount = tcp4buffer();
    // std::cout << buffercount.dump() << std::endl;
    json11::Json::object tcp4data_res{
        {"tcp4statcount", statcount},
        {"tcp4buffercount", buffercount}
    };
    return tcp4data_res;
}