#ifndef SECDETECTOR_OBSERVER_AGENT_GRPC_API_H
#define SECDETECTOR_OBSERVER_AGENT_GRPC_API_H

#include "comm_api.grpc.pb.h"
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
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;

#define MAX_CONNECTION 5

class PubSubServiceImpl final : public SubManager::Service
{
  public:
    grpc::Status Subscribe(ServerContext *context, const SubscribeRequest *request, ServerWriter<Message> *writer);
    grpc::Status Publish(ServerContext *context, const PublishRequest *request, Message *response);
    grpc::Status UnSubscribe(ServerContext *context, const UnSubscribeRequest *request, Message *response);

  private:
    std::unordered_map<std::string, std::vector<int>> suber_topic_;
    std::unordered_map<std::string, std::vector<ServerWriter<Message> *>> suber_writer_;
    std::unordered_map<std::string, std::vector<int>> suber_connection_;
    std::mutex sub_mutex;
    int connection_num = 0;
    bool connect_status[MAX_CONNECTION] = {false};
};

void StopServer();
void RunServer();

class PubSubClient
{
  public:
    PubSubClient();
    PubSubClient(std::shared_ptr<Channel> channel);
    void init(std::shared_ptr<Channel> channel);
    std::unique_ptr<ClientReader<Message>> Subscribe(const int topic);
    void Publish(const int topic, const std::string &content);
    void UnSubscribe(const int topic);
    std::string ReadFrom(std::unique_ptr<ClientReader<Message>> &reader);

  private:
    std::unique_ptr<SubManager::Stub> stub_;
    bool SubFlag;
    ClientContext context;
    std::string uuid_str;
};

#endif
