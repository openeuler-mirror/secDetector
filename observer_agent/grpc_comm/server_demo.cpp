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
 * Description: secDetector grpc server demo
 */
#include <grpcpp/grpcpp.h>
#include <sys/stat.h>
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
    int cli_topic = request->topic();
    std::string cli_name = request->sub_name();
    Message msg;

    for (auto iter = suber_topic_[cli_name].begin(); iter != suber_topic_[cli_name].end(); iter++) {
        if (*iter == cli_topic) {
            msg.set_text("this client name already subscribe the topic");
            if (!writer->Write(msg)) {
                std::cerr << "Failed to write the initial message" << std::endl;
                return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
            }
            return grpc::Status(grpc::StatusCode::INTERNAL, "this client name already subscribe the topic");
        }
    }

    suber_topic_[cli_name].push_back(cli_topic);
    suber_writer_[cli_name].push_back(writer);

    msg.set_text("topic: " + std::to_string(cli_topic) + " Subscribe success!");
    if (!writer->Write(msg)) {
        std::cerr << "Failed to write the initial message" << std::endl;
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
    }
    
    // ToDo: set some condition to break loop
    while (1) {}
    return grpc::Status::OK;
  }

    grpc::Status Publish(ServerContext* context, const PublishRequest* request, 
		      Message* response) override {
        int cli_topic = request->topic();
        std::string cli_data = request->data();
        int i = 0;
        Message msg;
        msg.set_text(cli_data);

        for (auto iter = suber_topic_.begin(); iter != suber_topic_.end(); iter++) {
            i = 0;
            for (auto topic_item : iter->second) {
                if (topic_item == cli_topic) {
                    auto& subscriber = suber_writer_[iter->first][i];
                    if (!subscriber->Write(msg)) {
                        std::cerr << "Failed to write to a subscriber" << std::endl;
                        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
                    }
                    break;
                }
                i++;
            }
        }

        return grpc::Status::OK;
  }

  grpc::Status UnSubscribe(ServerContext* context, const UnSubscribeRequest* request,
                           Message* response) override {
    int cli_topic = request->topic();
    std::string cli_name = request->sub_name();
    int i = 0;

    for (auto topic_item : suber_topic_[cli_name]) {
        if (topic_item == cli_topic) {
            suber_topic_[cli_name].erase(suber_topic_[cli_name].begin() + i);
            suber_writer_[cli_name].erase(suber_writer_[cli_name].begin() + i);
            break;
        }
        i++;
    }
    response->set_text("topic: " + std::to_string(cli_topic) + " UnSubscribe success!");
    return grpc::Status::OK;
  }
private:
    std::unordered_map<std::string, std::vector<int>> suber_topic_;
    std::unordered_map<std::string, std::vector<ServerWriter<Message>*>> suber_writer_;
};

void RunServer() {
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    chmod("/var/run/secDetector.sock", S_IRUSR | S_IWUSR);
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
