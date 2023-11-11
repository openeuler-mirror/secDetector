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
 * Create: 2023-10-18
 * Description: secDetector grpc client publish demo
 */
#include <grpcpp/grpcpp.h>
#include "comm_api.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using data_comm::Message;
using data_comm::SubManager;
using data_comm::SubscribeRequest;
using data_comm::UnSubscribeRequest;
using data_comm::PublishRequest;

#define BUF_NUM 1024

class PubSubClient {
    public:
    PubSubClient(std::shared_ptr<Channel> channel)
      : stub_(SubManager::NewStub(channel)) {}

    std::unique_ptr<ClientReader<Message>> Subscribe(const int topic, const std::string& name) {
        SubscribeRequest request;
        request.set_topic(topic);
        request.set_sub_name(name);

        Message msg;
        SubFlag = true;
        std::unique_ptr<ClientReader<Message>> reader = stub_->Subscribe(&context, request);
        if (reader == nullptr) {
            std::cerr << "Failed to subscribe." << std::endl;
            return nullptr;
        }
        return reader;
    }

    void Publish(const int topic, const std::string& content) {
        PublishRequest request;
        request.set_topic(topic);
        request.set_data(content);
        ClientContext pub_context;
	    Message msg;
	    grpc::Status status = stub_->Publish(&pub_context, request, &msg);
        if (!status.ok()) {
            std::cerr << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void UnSubscribe(const int topic, const std::string& name) {
        UnSubscribeRequest request;
        request.set_topic(topic);
        request.set_sub_name(name);

        ClientContext unsub_context;
        Message msg;
        grpc::Status status = stub_->UnSubscribe(&unsub_context, request, &msg);
        SubFlag = false;

        if (!status.ok()) {
            std::cerr << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
        } else {
            std::cout << "Received: " << msg.text() << std::endl;
        }
    }

    std::string ReadFrom(std::unique_ptr<ClientReader<Message>> &reader) {
        Message msg;
        if (reader->Read(&msg)) {
            std::cout << "Received: " << msg.text() << std::endl;
            return msg.text();
        } else {
            std::cerr << "Failed to read from the server." << std::endl;
            return ""; // Handle read error
        }
    }

    private:
        std::unique_ptr<SubManager::Stub> stub_;
        bool SubFlag;
        ClientContext context;
};

int main(int argc, char** argv) {
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubClient client(grpc::CreateChannel(
        server_address, grpc::InsecureChannelCredentials()));

    if (argc != 3) {
        std::cout << "[Usage] ./client_pub_demo topic_num publish_data" << std::endl;
    }
    client.Publish(std::stoi(argv[1]), argv[2]);
    sleep(3);
    client.Publish(std::stoi(argv[1]), "hello,world!");
    sleep(3);
    client.Publish(std::stoi(argv[1]), "end");

    return 0;
}

