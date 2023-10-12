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
using data_comm::Message;
using data_comm::SubManager;
using data_comm::SubscribeRequest;
using data_comm::UnSubscribeRequest;
using data_comm::PublishRequest;

class PubSubServiceImpl final : public SubManager::Service {
 public:
  grpc::Status Subscribe(ServerContext* context, const SubscribeRequest* request,
                         ServerWriter<Message>* writer) override {
    // Here you would normally check the topic and write the appropriate messages.
    // For simplicity, we're just writing a single message here.
    int cli_topic = request->topic();
    // ToDo: somebody topic
    subscribers_[cli_topic].push_back(writer);

    // ToDo: add extra check or feature code
    Message msg;
    msg.set_text("topic: " + std::to_string(cli_topic) + " Subscribe success!");
    writer->Write(msg);
    
    // ToDo: set some condition to break loop
    while (1) {}
    return grpc::Status::OK;
  }

  grpc::Status Publish(ServerContext* context, const PublishRequest* request, 
		      Message* response) override {
    int cli_topic = request->topic();
    std::string cli_data = request->data();

    if (subscribers_.find(cli_topic) != subscribers_.end()) {
        for (auto& subscriber : subscribers_[cli_topic]) {
	    Message msg;
	    msg.set_text(cli_data);
	    subscriber->Write(msg);
	}
    }
    return grpc::Status::OK;
  }

  grpc::Status UnSubscribe(ServerContext* context, const UnSubscribeRequest* request,
                           Message* response) override {
    int cli_topic = request->topic();
    //subscribers_[topic].pop_front(writer);

    // ToDo: add extra check or feature code
    response->set_text("topic: " + std::to_string(cli_topic) + " UnSubscribe success!");
    return grpc::Status::OK;
  }
 private:
  std::unordered_map<int, std::vector<ServerWriter<Message>*>> subscribers_;
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

