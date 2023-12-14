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
 * Description: secDetector grpc server
 */
#include "comm_api.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <sys/stat.h>
#include <condition_variable>

using data_comm::Message;
using data_comm::PublishRequest;
using data_comm::SubManager;
using data_comm::SubscribeRequest;
using data_comm::UnSubscribeRequest;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;

#define MAX_CONNECTION 5
#define CHECK_TIME 60

static bool killed = false;

class Subscribers {
public:
    int topic;
    ServerWriter<Message> *writer;

    Subscribers(int t, ServerWriter<Message> *w) : topic(t), writer(w) {}
    Subscribers() : topic(0), writer(nullptr) {}
};

class PubSubServiceImpl final : public SubManager::Service
{
  public:
    void CloseAllConnection(void)
    {
        std::lock_guard<std::mutex> lk(wait_mutex);

        killed = true;
        cv.notify_all();
    }

    grpc::Status Subscribe(ServerContext *context, const SubscribeRequest *request,
                           ServerWriter<Message> *writer) override
    {
        int cli_topic = request->topic();
        std::string cli_name = request->sub_name();
        Message msg;
        Message keepalive_msg;

        sub_mutex.lock();
        if (connection_num >= MAX_CONNECTION) {
            msg.set_text("over max connection number!");
            writer->Write(msg);
            sub_mutex.unlock();
            return grpc::Status(grpc::StatusCode::INTERNAL, "over max connection number");
        }

        auto iter = suber_topic_.find(cli_name);
        if (iter != suber_topic_.end()) {
            msg.set_text("this client name already subscribe the topic");
            writer->Write(msg);
            sub_mutex.unlock();
            return grpc::Status(grpc::StatusCode::INTERNAL, "this client name already subscribe the topic");
        }

        msg.set_text("topic: " + std::to_string(cli_topic) + " Subscribe success!");
        if (!writer->Write(msg))
        {
            std::cerr << "Failed to write the initial message" << std::endl;
            sub_mutex.unlock();
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
        }

        std::cout << "Subscribe " << cli_name << " ok" << std::endl;
        suber_topic_[cli_name] = Subscribers(cli_topic, writer);
        connection_num++;
        sub_mutex.unlock();

        /* loop until connot write */
        while (!killed)
        {
            sub_mutex.lock();
            if (suber_topic_.count(cli_name) == 0) {
                sub_mutex.unlock();
                return grpc::Status::OK;
            }

            keepalive_msg.set_text("keepalive");
            if (!writer->Write(keepalive_msg)) {
                DeleteSubscriberByCliName(cli_name);
                sub_mutex.unlock();
                return grpc::Status(grpc::StatusCode::INTERNAL, "writer is lose!");
            }
            sub_mutex.unlock();
            WaitKeeplive();
        }

        std::cout << cli_name << " is dead" << std::endl;
        return grpc::Status::OK;
    }

    grpc::Status Publish(ServerContext *context, const PublishRequest *request, Message *response) override
    {
        std::lock_guard<std::mutex> lock(sub_mutex);
        int cli_topic = request->topic();
        std::string cli_data = request->data();
        Message msg;
        msg.set_text(cli_data);

        for (auto iter = suber_topic_.begin(); iter != suber_topic_.end(); iter++)
        {
            Subscribers subscriber = iter->second;
            if ((subscriber.topic & cli_topic) != 0) {
                if (!subscriber.writer->Write(msg)) {
                    std::cerr << "Failed to write to a subscriber: " << iter->first << std::endl;
                    return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
                }
            }
        }

        return grpc::Status::OK;
    }

    grpc::Status UnSubscribe(ServerContext *context, const UnSubscribeRequest *request, Message *response) override
    {
        std::string cli_name = request->sub_name();
        std::lock_guard<std::mutex> lock(sub_mutex);

        if (connection_num <= 0) {
            response->set_text("connection_num <= 0, don't UnSubscribe!");
            
            return grpc::Status(grpc::StatusCode::INTERNAL, "connection_num <= 0, Failed to UnSubscribe topic!");
        }

        if (!DeleteSubscriberByCliName(cli_name))
        {
            response->set_text("don't exist the reader");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to UnSubscribe reader!");
        }
        response->set_text("UnSubscribe success!");
        return grpc::Status::OK;
    }

  private:
    std::unordered_map<std::string, Subscribers> suber_topic_;
    std::mutex sub_mutex;
    std::mutex wait_mutex;
    std::condition_variable cv;
    int connection_num = 0;

    void WaitKeeplive(void)
    {
        std::unique_lock<std::mutex> lk(wait_mutex);
        cv.wait_for(lk, std::chrono::seconds(CHECK_TIME), []{ return killed; });
    }

    /* Must called with sub_mutex */
    bool DeleteSubscriberByCliName(std::string &cli_name)
    {
        bool exist = false;
        std::cout << "UnSubscribe " << cli_name << " ok" << std::endl;

        auto it = suber_topic_.find(cli_name);
        if (it != suber_topic_.end()) {
            suber_topic_.erase(it);
            connection_num--;
            exist = true;
        }
        return exist;
    }
};

static std::unique_ptr<Server> server;
static PubSubServiceImpl service;

void StopServer()
{
    service.CloseAllConnection();
    server->Shutdown();
}

void RunServer()
{
    std::string server_address("unix:///var/run/secDetector.sock");

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = std::unique_ptr<Server>(builder.BuildAndStart());

    chmod("/var/run/secDetector.sock", S_IRUSR | S_IWUSR);
    server->Wait();
}
