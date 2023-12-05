#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include "json11.hh"
#include "server.hh"

#define DELAYTIME 1
// 延时秒数

std::thread backendThread([&]() {
    while (true) {
        auto data = get_data();
        std::ofstream file("sys_rtime.json", std::ios::out); 
        if (!file) {
            file.open("netinfo.json");
        }
        // std::cout << std::noshowbase << data << std::endl;
        std::string json = json11::Json(data).dump();
        // Remove backslashes
        json.erase(std::remove(json.begin(), json.end(), '\\'), json.end());
        // Remove leading and trailing double quotes
        if (!json.empty() && json.front() == '\"') {
            json.erase(json.begin());
        }
        if (!json.empty() && json.back() == '\"') {
            json.pop_back();
        }
        file << json;
        file.close();
        std::this_thread::sleep_for(std::chrono::seconds(DELAYTIME));
    }
});


int main(){
    backendThread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    std::cout << " main thread exit"  << std::endl;
    return 0;
}