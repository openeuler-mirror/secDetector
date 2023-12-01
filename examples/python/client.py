#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2023 Huawei Technologies Co., Ltd. All rights reserved.
secDetector is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details.

Author: zhangguanghzhi
Create: 2023-10-18
Description: secDetector example python client file
"""

import ctypes
import time
import threading
from threading import Thread

DATA_LEN = 1024

secDetectorsdklib = ctypes.cdll.LoadLibrary('/usr/lib64/secDetector/libsecDetectorsdk.so')

g_cli_reader = ctypes.c_void_p
g_cli_reader_lock = threading.Lock()

secDetectorsdklib.secSub.argtypes = [ctypes.c_int]
secDetectorsdklib.secSub.restype = ctypes.c_void_p
secDetectorsdklib.secUnsub.argtypes = [ctypes.c_int, ctypes.c_void_p]
secDetectorsdklib.secUnsub.restype = None
secDetectorsdklib.secReadFrom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
secDetectorsdklib.secReadFrom.restype = None

g_read_flag = True

def thread_func_sub_and_read(num=0):
    global g_cli_reader
    global g_read_flag

    cli_reader = secDetectorsdklib.secSub(1)
    g_cli_reader_lock.acquire()
    g_cli_reader = cli_reader
    g_cli_reader_lock.release()

    data = ctypes.create_string_buffer(DATA_LEN)
    data_len = ctypes.c_int(DATA_LEN)
    secDetectorsdklib.secReadFrom(cli_reader, data, data_len)
    print("client read data:{}".format(data.value.decode()))

    while g_read_flag:
        time.sleep(3)
        secDetectorsdklib.secReadFrom(cli_reader, data, data_len)
        print("client while read data:{}".format(data.value.decode()))

    print("client thread_func_sub_and_read end")

def thread_func_unsub(num=0):
    global g_cli_reader
    global g_read_flag

    g_cli_reader_lock.acquire()
    try:
        g_read_flag = False
        secDetectorsdklib.secUnsub(1, g_cli_reader)
    finally:
        g_cli_reader_lock.release()
    print("client thread_func_unsub end")

threadlist = []

tsub_read = Thread(target=thread_func_sub_and_read,args=(1,))
tsub_read.start()

time.sleep(5)
tunsub = Thread(target=thread_func_unsub,args=(2,))
tunsub.start()

threadlist.append(tsub_read)
threadlist.append(tunsub)

for t in threadlist:
    t.join()

print("client end")


