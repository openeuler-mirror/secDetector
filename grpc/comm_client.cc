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
#include <grpcpp/grpcpp.h>
#include "comm_api.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using pubsub::Message;
using pubsub::PubSub;
using pubsub::SubscribeRequest;

class PubSubClient {
    public:
    PubSubClient(std::shared_ptr<Channel> channel)
      : stub_(PubSub::NewStub(channel)) {}

    void Subscribe(const std::string& topic) {
        SubscribeRequest request;
        request.set_topic(topic);

        ClientContext context;

        std::unique_ptr<ClientReader<Message>> reader(
          stub_->Subscribe(&context, request));

        Message msg;
        while (reader->Read(&msg)) {
            std::cout << "Received: " << msg.text() << std::endl;
        }

        grpc::Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "Subscribe failed: " << status.error_message() << std::endl;
        }
}

    private:
        std::unique_ptr<PubSub::Stub> stub_;
};

int main(int argc, char** argv) {
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubClient client(grpc::CreateChannel(
        server_address, grpc::InsecureChannelCredentials()));

    client.Subscribe("chkmal");

    return 0;
}

