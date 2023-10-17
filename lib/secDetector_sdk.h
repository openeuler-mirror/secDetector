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
 * Author: zhangguangzhi
 * Create: 2023-10-8
 * Description: secDetector sdk file
 */

#ifndef SECDETECTOR_SDK_H
#define SECDETECTOR_SDK_H
/* file */
#define CREATFILE 0x00000001
#define DELFILE 0x00000002
#define SETFILEATTR 0x00000004
#define WRITEFILE 0x00000008
#define READFILE 0x00000010
/* process */
#define CREATPROCESS 0x00000020
#define DESTROYPROCESS 0x00000040
#define GETPROCESSATTR 0x00000080
#define SETPROCESSATTR 0x00000100
/* program behavior */
#define CREATEPIPE 0x00000200
#define EXCMD 0x00000400
#define CALLAPI 0x00000800
/* network */
#define ACCESSPORT 0x00001000
#define CONNECTNET 0x00002000
/* memory tamper */
#define PROCESSCODETAMPER 0x00004000
#define KERNELKEYDATATAMPER 0x00008000
/* resource consumption */
#define CPURESOURCECONSEME 0x00010000
#define MEMRESOURCECONSEME 0x00020000
#define STORAGERESOURCECONSEME 0x00040000
/* account management */
#define LOGINSUCCESS 0x00080000
#define LOGINFAILED 0x00100000
#define NEWACCOUNT 0x00200000
/* device management */
#define OPERATEDEV 0x00400000

void *secSub(const int topic);
void secUnsub(const int topic, void *reader);
const char *secReadFrom(void *reader)

#endif
