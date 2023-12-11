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

class PubSubServiceImpl final : public SubManager::Service
{
  public:
    void CloseAllConnection(void)
    {
	std::lock_guard<std::mutex> lk(wait_mutex);

        for (int i = 0; i < MAX_CONNECTION; i++) {
            connect_status[i] = false;
	}

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
        int i = 0, tmp_index;

        if (connection_num >= MAX_CONNECTION) {
            msg.set_text("over max connection number!");
            if (!writer->Write(msg))
            {
                std::cerr << "Failed to write the initial message" << std::endl;
                return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
            }
            return grpc::Status(grpc::StatusCode::INTERNAL, "over max connection number, Failed to Subscribe the topic");
        }

        for (auto iter = suber_topic_[cli_name].begin(); iter != suber_topic_[cli_name].end(); iter++)
        {
            if ((*iter & cli_topic) != 0)
            {
                msg.set_text("this client name already subscribe the topic");
                if (!writer->Write(msg))
                {
                    std::cerr << "Failed to write the initial message" << std::endl;
                    return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
                }
                return grpc::Status(grpc::StatusCode::INTERNAL, "this client name already subscribe the topic");
            }
        }

        sub_mutex.lock();

        for (tmp_index = 0; tmp_index <  MAX_CONNECTION; tmp_index++)
        {
            if (!connect_status[tmp_index])
                break;
        }

        if (tmp_index == MAX_CONNECTION)
        {
            sub_mutex.unlock();
            msg.set_text("multi-process max connection number!");
            if (!writer->Write(msg))
            {
                std::cerr << "Failed to write the initial message" << std::endl;
                return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
            }
            return grpc::Status(grpc::StatusCode::INTERNAL, "multi-process max connection number, Failed to Subscribe the topic");
        }

        msg.set_text("topic: " + std::to_string(cli_topic) + " Subscribe success!");
        if (!writer->Write(msg))
        {
            std::cerr << "Failed to write the initial message" << std::endl;
            sub_mutex.unlock();
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write the message");
        }

        suber_topic_[cli_name].push_back(cli_topic);
        suber_writer_[cli_name].push_back(writer);
        suber_connection_[cli_name].push_back(tmp_index);
        connect_status[tmp_index] = true;
        connection_num++;

        sub_mutex.unlock();

        keepalive_msg.set_text("keepalive");
        while (connect_status[tmp_index])
        {
            if (!writer->Write(keepalive_msg))
            {
                for (auto topic_item : suber_topic_[cli_name])
                {
                    if (topic_item == cli_topic)
                    {
                        sub_mutex.lock();
                        suber_topic_[cli_name].erase(suber_topic_[cli_name].begin() + i);
                        suber_writer_[cli_name].erase(suber_writer_[cli_name].begin() + i);
                        connect_status[suber_connection_[cli_name].at(i)] = false;
                        suber_connection_[cli_name].erase(suber_connection_[cli_name].begin() + i);
                        connection_num--;
                        sub_mutex.unlock();
                        break;
                    }
                    i++;
                }
                return grpc::Status(grpc::StatusCode::INTERNAL, "writer is lose!");
            }
            WaitKeeplive();
        }
        return grpc::Status::OK;
    }

    grpc::Status Publish(ServerContext *context, const PublishRequest *request, Message *response) override
    {
        int cli_topic = request->topic();
        std::string cli_data = request->data();
        int i = 0;
        Message msg;
        msg.set_text(cli_data);

        for (auto iter = suber_topic_.begin(); iter != suber_topic_.end(); iter++)
        {
            i = 0;
            for (auto topic_item : iter->second)
            {
                if ((topic_item & cli_topic) != 0)
                {
                    auto &subscriber = suber_writer_[iter->first][i];
                    if (!subscriber->Write(msg))
                    {
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

    grpc::Status UnSubscribe(ServerContext *context, const UnSubscribeRequest *request, Message *response) override
    {
        std::string cli_name = request->sub_name();
        int i = 0;
        int unsub_flag = 0;

        if (connection_num <= 0) {
            response->set_text("connection_num <= 0, don't UnSubscribe!");
            
            return grpc::Status(grpc::StatusCode::INTERNAL, "connection_num <= 0, Failed to UnSubscribe topic!");
        }

        std::lock_guard<std::mutex> lock(sub_mutex);

        std::unordered_map<std::string, std::vector<int>>::iterator iter = suber_topic_.find(cli_name);
        if (iter != suber_topic_.end())
        {
            suber_topic_[cli_name].erase(suber_topic_[cli_name].begin() + i);
            suber_writer_[cli_name].erase(suber_writer_[cli_name].begin() + i);
            connect_status[suber_connection_[cli_name].at(i)] = false;
            suber_connection_[cli_name].erase(suber_connection_[cli_name].begin() + i);
            connection_num--;
            unsub_flag = 1;
        }

        if (!unsub_flag)
        {
            response->set_text("don't exist the reader");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to UnSubscribe reader!");
        }
        response->set_text("UnSubscribe success!");
        return grpc::Status::OK;
    }

  private:
    std::unordered_map<std::string, std::vector<int>> suber_topic_;
    std::unordered_map<std::string, std::vector<ServerWriter<Message> *>> suber_writer_;
    std::unordered_map<std::string, std::vector<int>> suber_connection_;
    std::mutex sub_mutex;
    std::mutex wait_mutex;
    std::condition_variable cv;
    int connection_num = 0;
    bool connect_status[MAX_CONNECTION] = {false};

    void WaitKeeplive(void)
    {
	std::unique_lock<std::mutex> lk(wait_mutex);
	cv.wait_for(lk, std::chrono::seconds(CHECK_TIME), []{ return killed; });
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
