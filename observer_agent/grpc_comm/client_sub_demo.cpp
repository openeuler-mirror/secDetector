/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 *
 * Author: hurricane618
 * Create: 2023-09-26
 * Description: secDetector grpc client subscribe demo
 */
#include "grpc_api.h"

int main(int argc, char** argv) {
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubClient client(grpc::CreateChannel(
        server_address, grpc::InsecureChannelCredentials()));

    if (argc != 2) {
        std::cout << "[Usage] ./client_sub_demo topic_num" << std::endl;
        return 0;
    }

    std::unique_ptr<ClientReader<Message>> cli_reader = client.Subscribe(std::stoi(argv[1]));
    std::string some_data = client.ReadFrom(cli_reader);
    std::cout << "whz: " << some_data << std::endl;
    while (some_data != "" && some_data != "end") {
        some_data = client.ReadFrom(cli_reader);
        std::cout << "loop whz: " << some_data << std::endl;
    }
    client.UnSubscribe(std::stoi(argv[1]));

    return 0;
}

