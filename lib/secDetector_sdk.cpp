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
 * Author: zhangguanghzhi
 * Create: 2023-10-8
 * Description: secDetector sdk file
 */

#include <string>
#include <iostream>
#include "../observer_agent/grpc_comm/grpc_api.h"

using namespace std;
static string server_address("unix:///var/run/secDetector.sock");
static PubSubClient g_client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials));

using Readmap = map<void *, unique_ptr<ClientReader<Message>>>;
static Readmap g_reader_map;

PubSubClient::PubSubClient(shared_ptr<Channel> channel)
	: stub_(SubManager::NewStub(channel)) {}

unique_ptr<ClientReader<Message>> PubSubClient::Subscribe(const int topic) 
{
	SubscribeRequest request;
	request.set_topic(topic);

	return stub_->Subscribe(&context, request);
}

void PubSubClient::Publish(const int topic, const string &context)
{
	PublishRequest request;
	request.set_topic(topic);
	request.set_data(context);

	ClientContext pub_context;
	Message msg;

	grpc::Status status = stub_->Publish(&pub_context, request, &msg);

	if (!status.ok()) {
		cerr << "Publish Error: " << status.error.code() << ": " << status.error_message() << endl;
	}
}

void PubSubClient::UnSubscribe(const int topic)
{
	UnSubscribeRequest request;
	request.set_topic(topic);

	ClientContext context;
	Message msg;
	grpc::Status status = stub_->UnSubscibe(&context, request, &msg);

	SubFlag = false;

	if (!status.ok()) {
                cerr << "UnSubscribe Error: " << status.error.code() << ": " << status.error_message() << endl;
        }

	cout << "UnSubsccribe Received: " << msg.text() << endl;
}

string PubSubClient::ReadFrom(unique_ptr<ClientReader<Message>> &reader)
{
	Message msg;
	reader->Read(&msg);
	cout << "Received: " << msg.text() << endl;
	return msg.text();
}



#ifdef __cplusplus
extern "C" {
#endif

void *secSub(const int topic)
{
	unique_ptr<ClientReader<Message>> reader = g_clinet.Subscribe(topic);
	void * ret_reader = static<void *>(reader.get());

	g_reader_map.insert(Readmap::value_type(ret_reader, move(reader)));
	return ret_reader;
}

void secUnsub(const int topic, void *reader)
{
	g_client.Publih(topic, "end");
	g_client.UnSubscribe(topic);
	
	Readmao::iterator iter = g_reader_map.find(reader);
	if (iter != g_reader_map.end()) {
		g_reader_map.erase(iter);
	}
}

void secReadFrom(void *reader, char *data, int data_len)
{
	string msg("");

	Readmao::iterator iter = g_reader_map.find(reader);
        if (iter != g_reader_map.end()) {
		msg = g_client.ReadFrom(iter->second);
        }

	strncpy(data, msg.c_str(), data_len - 1);
}

#ifdef __cplusplus
}
#endif

