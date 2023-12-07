#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include "json11.hh"

struct ConnectionInfo {
    std::string sourceIpAddress;
    std::string sourcePort;
};

std::vector<ConnectionInfo> getSSHConnections() {
    std::vector<ConnectionInfo> connections;

    // 执行netstat命令获取SSH ESTABLISHED连接信息
    FILE* netstat_pipe = popen("netstat -ltnpa 2>/dev/null | grep :22 | grep ESTABLISHED", "r");
    if (!netstat_pipe) {
        std::cerr << "Error executing netstat command." << std::endl;
        return connections;
    }

    // 读取netstat命令输出并解析连接信息
    char line[256];
    while (fgets(line, sizeof(line), netstat_pipe) != nullptr) {
        std::istringstream iss(line);
        std::string token;

        ConnectionInfo connection;

        // 使用空格分割字符串
        for (int i = 0; i < 5; ++i) {
            iss >> token;
            if (i == 4) {
                size_t pos = token.find_last_of(':');
                if (pos != std::string::npos) {
                    connection.sourceIpAddress = token.substr(0, pos);
                    connection.sourcePort = token.substr(pos + 1);
                }
            }
        }

        connections.push_back(connection);
    }

    pclose(netstat_pipe);

    return connections;
}

std::vector<ConnectionInfo> getLoggedInUserConnections() {
    std::vector<ConnectionInfo> connections;

    // 执行last命令获取已连接但未注销的SSH连接信息，剔除tty登录方式
    FILE* last_pipe = popen("last | grep 'still logged in' | awk '!/tty/'", "r");
    if (!last_pipe) {
        std::cerr << "Error executing last command." << std::endl;
        return connections;
    }

    // 读取last命令输出并解析连接信息
    char line[256];
    while (fgets(line, sizeof(line), last_pipe) != nullptr) {
        std::istringstream iss(line);
        std::string username, pts, ipAddress;

        ConnectionInfo connection;

        
        // 使用空格分割字符串
        iss >> username >> pts >> ipAddress;
        // 仅添加IPv4地址
        if (ipAddress.find('.') != std::string::npos) {
            size_t pos = ipAddress.find_last_of(':');
            if (pos != std::string::npos) {
                connection.sourceIpAddress = ipAddress.substr(0, pos);
                connection.sourcePort = ipAddress.substr(pos + 1);
            } else {
                // 如果无法找到端口号，设置为空字符串
                connection.sourceIpAddress = ipAddress;
                connection.sourcePort = "";
            }
        }

        connections.push_back(connection);
    }

    pclose(last_pipe);

    return connections;
}

json11::Json  getSSHInfoAll() {
    std::vector<ConnectionInfo> sshConnections = getSSHConnections();
    std::vector<ConnectionInfo> loggedInUserConnections = getLoggedInUserConnections();

    // 存储未验证的SSH连接数量和IP地址
    std::vector<ConnectionInfo> unauthenticatedConnections;

    // 遍历netstat命令获取的数组，剔除last命令获取的数组
    for (const auto& sshConnection : sshConnections) {
        auto it = std::find_if(loggedInUserConnections.begin(), loggedInUserConnections.end(),
                               [&sshConnection](const ConnectionInfo& loggedInConnection) {
                                   return sshConnection.sourceIpAddress == loggedInConnection.sourceIpAddress ;
                               });

         // 如果未在last命令获取的数组中找到，则添加到未验证的SSH连接数组
        if (it != loggedInUserConnections.end()) {
            loggedInUserConnections.erase(it);
        } else {
            unauthenticatedConnections.push_back(sshConnection);
        }
    }

    json11::Json::object resultJson;
    resultJson["UnauthenticatedSSHConnectionCount"] = static_cast<int>(unauthenticatedConnections.size());


    json11::Json::array unauthenticatedConnectionsArray;
    for (const auto& connection : unauthenticatedConnections) {
        unauthenticatedConnectionsArray.push_back(json11::Json::object{
            {"sourceIpAddress", connection.sourceIpAddress},
            {"sourcePort", connection.sourcePort}
        });
    }
    resultJson["UnauthenticatedSSHConnections"] = unauthenticatedConnectionsArray;

    std::string jsonString = json11::Json(resultJson).dump();
    std::cout << jsonString << std::endl;

    return resultJson;
}
