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

#define ALLTOPIC 0x00FFFFFF
using namespace std;
static string server_address("unix:///var/run/secDetector.sock");
static PubSubClient g_client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

using Readmap = map<void *, unique_ptr<ClientReader<Message>>>;
static Readmap g_reader_map;

#ifdef __cplusplus
extern "C" {
#endif

void *secSub(const int topic)
{
	if (topic <= 0 || topic > ALLTOPIC) {
		printf("secSub failed, topic:%d is error\n", topic);
		return NULL;
	}

	unique_ptr<ClientReader<Message>> reader = g_client.Subscribe(topic);

	if (!reader)
		return NULL;
	void * ret_reader = static_cast<void *>(reader.get());

	g_reader_map.insert(Readmap::value_type(ret_reader, move(reader)));
	return ret_reader;
}

void secUnsub(const int topic, void *reader)
{
	if (topic <= 0 || topic > ALLTOPIC) {
		printf("secUnsub failed, topic:%d is error\n", topic);
		return;
	}

	if (!reader)
		return;

	g_client.UnSubscribe(topic);
	
	Readmap::iterator iter = g_reader_map.find(reader);
	if (iter != g_reader_map.end()) {
		g_reader_map.erase(iter);
		reader = NULL;
	}
}

void secReadFrom(void *reader, char *data, int data_len)
{
	string msg("");

	if (!data || data_len <= 1)
		return

	memset(data, 0, data_len);

	if (!reader)
		return;

	Readmap::iterator iter = g_reader_map.find(reader);
	if (iter != g_reader_map.end()) {
		msg = g_client.ReadFrom(iter->second);
		if (msg == "keepalive")
			return;
	}

	strncpy(data, msg.c_str(), data_len - 1);
}

#ifdef __cplusplus
}
#endif

