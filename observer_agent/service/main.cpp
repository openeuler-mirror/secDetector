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
 * Author: chenjingwen
 * Create: 2023-09-25
 * Description: secDetector main entry
 */
#include "../grpc_comm/grpc_api.h"
#include "ebpf/ebpf_types.h"
#include "ebpf/fentry.h"
#include "ebpf_converter.h"
#include "ringbuffer.h"
#include "secDetector_topic.h"
#include <errno.h>
#include <iostream>
#include <limits>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <thread>

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

#define MIN_RINGBUF_SIZE 4
#define MAX_RINGBUF_SIZE 1024
static unsigned int ringbuf_size_bytes;
static unsigned int ringbuf_size = MIN_RINGBUF_SIZE;
static int topic_mask = ALL_TOPIC;

static bool power_of_2(unsigned int num)
{
    if (num == 0)
        return false;
    if ((num & (num - 1)) != 0)
        return false;
    return true;
}

static bool ringbuf_size_check(void)
{
	if (ringbuf_size < MIN_RINGBUF_SIZE || ringbuf_size > MAX_RINGBUF_SIZE) {
		std::cerr << "[secDetector] ringbuf_size should be 4 and 1024 (Mb)" << std::endl;
		return false;
	}

	if (!power_of_2(ringbuf_size)) {
		std::cerr << "[secDetector] ringbuf_size should be power of 2" << std::endl;
		return false;
	}

    std::cout << "ringbuf size is set to " << ringbuf_size << "Mb" << std::endl;
	ringbuf_size_bytes = ringbuf_size * 1024 * 1024;
	return true;
}

static volatile sig_atomic_t exiting = 0;
static void sig_handler(int sig)
{
    exiting = true;
}
static bool debug = false;

static void push_log(int type, const std::string &content)
{
    if ((topic_mask & type)  == 0)
        return;

    // push to console if debug
    if (debug)
    {
        std::cout << "type:" << type << " content:" << content << std::endl;
    }

    // push to grpc
    std::string server_address("unix:///var/run/secDetector.sock");
    PubSubClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    client.Publish(type, content);
}

static void set_signal_handler(void)
{
    struct sigaction action;
    action.sa_handler = sig_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
}

static int ringbuf_cb(struct response_rb_entry *entry, size_t entry_size)
{
    if (entry == NULL || entry_size <= sizeof(struct response_rb_entry))
        return -EINVAL;
    push_log(entry->type, entry->text);
    return 0;
}

static int ebpf_cb(void *ctx, void *data, size_t data_sz)
{
    struct ebpf_event *e = (ebpf_event *)data;

    push_log(e->type, ebpf_event_to_text(e));
    return 0;
}

int main(int argc, char *argv[])
{
    int r;
    int opt;

    set_signal_handler();
    while ((opt = getopt(argc, argv, "ds:t:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debug = true;
            break;
        case 's':
            ringbuf_size = strtoul(optarg, NULL, 0);
            break;
        case 't':
            topic_mask = strtoul(optarg, NULL, 0);
            break;
        }
    }

    if (topic_mask == 0) {
        std::cerr << "not a valid topic_mask" << std::endl;
    	return -1;
    }

    if (!ringbuf_size_check()) {
        std::cerr << "Not a valid ring buffer size" << std::endl;
        return -1;
    }

    if (debug)
    {
        printf("Run in debug mode, log will be printed to console too.\n");
    }
    else
    {
        r = daemon(0, 0);
        if (r == -1)
        {
            printf("daemon failed, r:%d\n");
            exit(EXIT_FAILURE);
        }
    }

    r = secDetector_ringbuf_attach();
    if (r != 0)
    {
        std::cerr << "cannot attach to ringbuffer, r:" << r << std::endl;
        exit(EXIT_FAILURE);
    }

    std::thread thread_grpc = std::thread(RunServer);
    std::thread thread_ebpf_process = std::thread(StartProcesseBPFProg, ebpf_cb, ringbuf_size_bytes, topic_mask);
    std::thread thread_ebpf_file = std::thread(StartFileBPFProg, ebpf_cb, ringbuf_size_bytes, topic_mask);

    while (exiting == 0)
    {
        r = secDetector_ringbuf_poll((poll_cb)ringbuf_cb);
        if (r != 0)
        {
            std::cerr << "secDetector_ringbuf_poll failed, r:" << r << std::endl;
            break;
        }
    }

    secDetector_ringbuf_detach();
    std::cout << "Wait grpc server shutdown" << std::endl;
    StopServer();
    thread_grpc.join();

    std::cout << "Wait ebpf process program detached" << std::endl;
    StopProcesseBPFProg();
    thread_ebpf_process.join();

    std::cout << "Wait ebpf file program detached" << std::endl;
    StopFileBPFProg();
    thread_ebpf_file.join();
    return 0;
}
