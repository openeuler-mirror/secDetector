/*
 * Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 * secDetector is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 *
 * Author: hurricane618
 * Create: 2023-10-18
 * Description: secDetector grpc client publish demo
 */
#include "grpc_api.h"

int main(int argc, char **argv)
{
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    if (argc != 3)
    {
        std::cout << "[Usage] ./client_pub_demo topic_num publish_data" << std::endl;
        return 0;
    }
    client.Publish(std::stoi(argv[1]), argv[2]);
    sleep(3);
    client.Publish(std::stoi(argv[1]), "hello,world!");
    sleep(3);
    client.Publish(std::stoi(argv[1]), "end");

    return 0;
}
