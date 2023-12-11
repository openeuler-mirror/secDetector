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
 * Description: secDetector grpc client
 */
#include "comm_api.grpc.pb.h"
#include "grpc_api.h"
#include <grpcpp/grpcpp.h>
#include <uuid/uuid.h>

using data_comm::Message;
using data_comm::PublishRequest;
using data_comm::SubManager;
using data_comm::SubscribeRequest;
using data_comm::UnSubscribeRequest;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;

#define BUF_NUM 1024

PubSubClient::PubSubClient() {}

PubSubClient::PubSubClient(std::shared_ptr<Channel> channel) : stub_(SubManager::NewStub(channel))
{
    uuid_t uuid;
    char uuid_temp[37];
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_temp);
    uuid_str = std::string(uuid_temp);
}

void PubSubClient::init(std::shared_ptr<Channel> channel)
{
    uuid_t uuid;
    char uuid_temp[37];
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_temp);
    uuid_str = std::string(uuid_temp);
    stub_ = SubManager::NewStub(channel);
}

std::unique_ptr<ClientReader<Message>> PubSubClient::Subscribe(const int topic)
{
    SubscribeRequest request;
    request.set_topic(topic);
    request.set_sub_name(uuid_str);
    std::string ret_info;

    Message msg;
    SubFlag = true;
    std::unique_ptr<ClientReader<Message>> reader = stub_->Subscribe(&context, request);
    ret_info = ReadFrom(reader);

    if (ret_info.substr(0, 6) == "topic:")
    {
        std::cout << "Success subscribe." << std::endl;
        return reader;
    } else
    {
        std::cerr << "Failed to subscribe." << std::endl;
        return nullptr;
    }
}

void PubSubClient::Publish(const int topic, const std::string &content)
{
    PublishRequest request;
    request.set_topic(topic);
    request.set_data(content);
    ClientContext pub_context;
    Message msg;
    grpc::Status status = stub_->Publish(&pub_context, request, &msg);
    if (!status.ok())
    {
        std::cerr << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
    }
}

void PubSubClient::UnSubscribe(void)
{
    UnSubscribeRequest request;
    request.set_sub_name(uuid_str);

    ClientContext unsub_context;
    Message msg;
    grpc::Status status = stub_->UnSubscribe(&unsub_context, request, &msg);
    SubFlag = false;

    if (!status.ok())
    {
        std::cerr << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
    }
    else
    {
        std::cout << "Received: " << msg.text() << std::endl;
    }
}

std::string PubSubClient::ReadFrom(std::unique_ptr<ClientReader<Message>> &reader)
{
    Message msg;
    if (reader->Read(&msg))
    {
        std::cout << "Received: " << msg.text() << std::endl;
        return msg.text();
    }
    else
    {
        std::cerr << "Failed to read from the server." << std::endl;
        return ""; // Handle read error
    }
}
