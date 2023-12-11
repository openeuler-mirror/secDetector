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
#include <list>
#include <mutex>
#include "../observer_agent/grpc_comm/grpc_api.h"

#define ALLTOPIC 0x00FFFFFF
using namespace std;
static string server_address("unix:///var/run/secDetector.sock");

using Readmap = map<void *, std::pair<unique_ptr<ClientReader<Message>>, PubSubClient *>>;
static Readmap g_reader_map;
static mutex g_connect_mtx;

#ifdef __cplusplus
extern "C" {
#endif

void *secSub(const int topic)
{
	PubSubClient *cur_client;
	if (topic <= 0 || topic > ALLTOPIC) {
		printf("lib secSub failed, topic:%d is error\n", topic);
		return NULL;
	}
	g_connect_mtx.lock();
	std::shared_ptr<Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
	cur_client = new(PubSubClient);
	if (cur_client == nullptr) {
		g_connect_mtx.unlock();
		return NULL;
	}
	cur_client->init(channel);
	unique_ptr<ClientReader<Message>> reader = cur_client->Subscribe(topic);

	if (!reader) {
		printf("lib secSub failed, get reader null\n");
		delete cur_client;
		g_connect_mtx.unlock();
		return NULL;
	}
	void * ret_reader = static_cast<void *>(reader.get());

	g_reader_map.insert(Readmap::value_type(ret_reader, std::make_pair(move(reader), cur_client)));
	g_connect_mtx.unlock();
	return ret_reader;
}

void secUnsub(void *reader)
{
	PubSubClient *cur_client;

	if (!reader)
		return;

	g_connect_mtx.lock();
	Readmap::iterator iter = g_reader_map.find(reader);
	if (iter != g_reader_map.end()) {
		cur_client = iter->second.second;
		cur_client->UnSubscribe();
		g_reader_map.erase(iter);
		reader = NULL;
		delete cur_client;
	}
	g_connect_mtx.unlock();
}

void secReadFrom(void *reader, char *data, int data_len)
{
	string msg("");
	PubSubClient *cur_client;

	if (!data || data_len <= 1)
		return

	(void)memset(data, 0, data_len);

	if (!reader)
		return;

	g_connect_mtx.lock();
	Readmap::iterator iter = g_reader_map.find(reader);
	if (iter != g_reader_map.end()) {
		cur_client = iter->second.second;
		msg = cur_client->ReadFrom(iter->second.first);
		if (msg == "keepalive") {
			g_connect_mtx.unlock();
			return;
		}
		strncpy(data, msg.c_str(), data_len - 1);
	}
	g_connect_mtx.unlock();
}

#ifdef __cplusplus
}
#endif

