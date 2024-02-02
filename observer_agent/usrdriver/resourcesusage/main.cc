#include "json11.hh"
#include "server.hh"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>

#define DELAYTIME 1
#define RING_BUFFER_SIZE 1000
// 延时秒数

std::thread backendThread([]() {
    while (true) {
        auto data = get_data();
        std::ofstream file("sys_rtime.json", std::ios::out);
        if (!file)
            file.open("netinfo.json");
        std::string json = json11::Json(data).dump();
        json.erase(std::remove(json.begin(), json.end(), '\\'), json.end());
        if (!json.empty() && json.front() == '\"')
            json.erase(json.begin());
        if (!json.empty() && json.back() == '\"')
            json.pop_back();
        file << json;
        file.close();
        std::this_thread::sleep_for(std::chrono::seconds(DELAYTIME));
    }
});

void convertToJsonString(const json11::Json &obj, const std::string &prefix, std::vector<std::string> &result)
{
    if (obj.is_object()){
        for (const auto &kv : obj.object_items()){
            std::string newPrefix = prefix.empty() ? kv.first : prefix + "->" + kv.first;
            convertToJsonString(kv.second, newPrefix, result);
        }
    }
    else if (obj.is_array())
    {
        int index = 0;
        for (const auto &item : obj.array_items()){
            convertToJsonString(item, prefix + std::to_string(index), result);
            index++;
        }
    }
    else
        result.push_back(prefix + ":" + obj.dump() + ";");
}

std::vector<std::string> convertJson(const json11::Json &input)
{
    std::vector<std::string> result;
    convertToJsonString(input, "", result);
    return result;
}

#include <sstream>

int main(){
    // 后台线程
    backendThread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while(true){
        std::string filename = "sys_rtime.json"; // Specify the filename here
        std::ifstream file(filename);

        if (!file.is_open())
        {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return 1;
        }

        // Read the entire file contents into a string
        std::string jsonString;
        file.seekg(0, std::ios::end);
        jsonString.reserve(file.tellg());
        file.seekg(0, std::ios::beg);
        jsonString.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::string err;
        const auto json = json11::Json::parse(jsonString, err);

        if (!err.empty())
        {
            std::cerr << "Error parsing JSON: " << err << std::endl;
            return 1;
        }
        file.close();
        auto convertedJson = convertJson(json);

        for (const auto &line : convertedJson)
        {
            std::cout << line << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(DELAYTIME));
    }

    return 0;
}