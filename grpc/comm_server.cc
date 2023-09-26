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
 * Description: secDetector grpc server
 */
#include <grpcpp/grpcpp.h>
#include "comm_api.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using pubsub::Message;
using pubsub::PubSub;
using pubsub::SubscribeRequest;

class PubSubServiceImpl final : public PubSub::Service {
 public:
  grpc::Status Subscribe(ServerContext* context, const SubscribeRequest* request,
                         ServerWriter<Message>* writer) override {
    // Here you would normally check the topic and write the appropriate messages.
    // For simplicity, we're just writing a single message here.
    std::string cli_topic = request->topic();
    Message msg;
    if (cli_topic == "chkmal") {
        msg.set_text("our probe data");
    } else {
        msg.set_text("Hello, world!");
    }
    writer->Write(msg);
    return grpc::Status::OK;
  }
};

void RunServer() {
  std::string server_address("unix:///var/run/secDetector.sock");
  PubSubServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}

