# secDetector



## 介绍

Operating System Security Intrusion Detection System

secDetector 是专为OS设计的内构入侵检测系统，旨在为关键信息基础设施提供入侵检测及响应能力，为第三方安全工具减少开发成本同时增强检测和响应能力。secDetector 基于ATT&CK攻击模式库建模提供更为丰富的安全事件原语，并且可以提供实时阻断和灵活调整的响应能力。

secDetector 作为一套灵活的OS内构入侵检测系统，有三种使用模式：

1. 直接被系统用户开启用作一些基础异常事件的告警和处置。
2. 被安全态势感知服务集成，补齐系统信息采集缺陷，用于APT等复杂的安全威胁分析和重点事件布控实时阻断。
3. 由安全从业人员或安全感知服务提供商二次开发，基于可拓展框架构建精确、高效、及时的入侵检测与响应能力。

## 软件架构

```
||==APP===================================================================||
||                                                                        ||
||                     ----------------------------                       ||
||                     |           SDK            |                       ||
||                     ----------------------------                       ||
||                                 /^\                                    ||
||==================================|=====================================||
                                    |
                                    |
                                    |
||==OBSERVER========================|=====================================||
||                                  |                                     ||
||                     ----------------------------                       ||
||                     |         service          |                       ||
||                     ----------------------------                       ||
||                                 /^\                                    ||
||==================================|=====================================||
                                    |
||==DRIVER================================================================||
||                                                                        ||
||                     ----------------------------                       ||
||                     |     8 types of cases     |                       ||
||                     ----------------------------                       ||
||                                                                        ||
||------------------------------------------------------------------------||
||                                                                        ||
||  -------------  ----------------  ----------------  -----------------  ||
||  | hook unit |  | collect unit |  | analyze unit |  | response unit |  ||
||  -------------  ----------------  ----------------  -----------------  ||
||                                                                        ||
||========================================================================||
```

secDetector在架构上分为四个部分：SDK、service、检测特性集合cases、检测框架core。

- SDK

  SDK是以一个用户态动态链接库lib的形态承载，被部署到需要使用secDetector入侵检测系统的安全感知业务中。SDK用于和secDetector入侵检测系统的service通讯，完成所需的工作（例如订阅，去订阅，读取现有消息等功能）。secDetector提供的异常信息被定义成不同的case，安全感知业务可以根据自身需求订阅。

- service

  service是以一个用户态服务应用的形态承载，向上管理、维护安全感知业务的case订阅信息，向下维护case的运行情况。框架core和检测特性集合case采集到的信息由service统一收集，按需转发给不同的安全感知业务。安全感知业务对于底层检测特性集合case和框架core的配置、管理的需求也由service进行统一的管理和转发。不同的安全感知业务可能会需求同样的case，service会统计出所有安全感知业务需求case的并集，向下层注册。

- 特性集合cases

  检测特性集合cases是一系列异常检测探针，根据异常信息的不同会有不同的形态，比如内核异常信息检测的每个探针会以内核模块ko的形态承载。一个case代表一个探针，一个探针往往是一类异常信息或者一类异常事件的信息。比如进程类探针会关注所有进程的创建、退出、属性修改等事件信息，内存修改类探针会收集内核模块列表和安全开关等信息。因此一个探针可能会包含对多个事件的监控，而这些对不同事件的监控逻辑可能无法部署在同一个执行流当中。我们使用工作流（workflow）的概念表示一个探针在同一个执行流中的工作，一个探针可以包含一个或者多个工作流。比如对于进程探针而言，进程创建检测和进程属性修改检测就是不同的工作流。

- 框架core

  检测框架core是每一个case依赖的基础框架，提供case的管理和workflow所需的通用的基础功能单元。内核异常信息检测框架会以内核模块ko的形态承载。一个检测特性case可以将自己注册到框架中，或者从框架中去注册。框架还可以提供特定的交互接口以满足外部的动态请求。一个workflow被定义为有四类功能单元组成：事件发生器、信息采集器、事件分析器、响应单元。

特性集合cases和框架core合起来被称为driver。driver驱动提供了secDetector功能的最底层的系统级实现。
driver分为两类，kerneldriver 和 usrdriver。顾名思义，kerneldriver是部署在内核态中的，以内核模块的形式承载。usrdriver是部署在用户态中的，直接被部署为service中的一个模块。从逻辑上usrdriver是在service之下的，但是在运行中，为了降低通信成本，usrdriver被直接集成在service程序中。

## 安装教程
- kerneldriver

   1. 安装编译依赖 #yum install kernel-devel kernel-headers
   2. core目录下执行make，并插入core模块 #insmod secDetector_core.ko
   3. cases目录下执行make，按需插入需要的模块 如 #insmod secDetector_kmodule_baseline.ko

- observer_agent

   1. 安装编译依赖 #yum install gcc gcc-c++ clang libbpf-devel bpftool grpc-devel grpc-plugins cmake make protobuf-devel c-ares-devel libuuid-devel
   2. mkdir -p build && cd build
   3. cmake .. && make
   4. 执行服务程序 # ./secDetectord &

- lib

   1. mkdir -p lib/build && cd lib/build
   2. cmake .. && make
   3. 建议库文件部署到/usr/lib64/secDetector/libsecDetectorsdk.so

以上3部分按顺序构建并部署完之后，可以调用libsecDetectorsdk.so提供的api接口开发应用

在支持rpm包安装环境，直接安装openEuler版本中的secDetector-xxx.rpm和开发包secDetector-devel-xxx.rpm，即可完成部署

## 使用说明
### 运行 secDetectord 服务
```
用法：secDetectord [选项]
secDetectord 默认会在后台运行，从探针中取得数据并转发给订阅者
选项：
  -d         进入调试模式，进入前台运行，并且在控制台打印探针数据。
  -s <size>  配置eBPF缓冲区大小，单位为Mb，默认为4； size可选范围为4~1024，且必须为2的幂次方。当前拥有2个独立的缓冲区。
  -t <topic> 支持配置订阅的事件，默认为所有事件。topic 是位图格式。例如 -t 0x60 同时订阅进程创建和进程退出事件。详细请查阅 include/secDetector_topic.h。
```

### api接口说明

接口名称	void *secSub(const int topic)
接口描述	注册订阅接口
参数	“topic”:注册的事件类型，具体可见” /usr/include/secDetector/secDetector_sdk.h”中定义
输出	返回读取事件的指针

接口名称	void secUnsub(void *reader)
接口描述	注销订阅接口
参数	“reader”:注销的读事件指针
输出	无
注意事项	当前会取消reader的全部订阅

接口名称	void secReadFrom(void *reader, char *data, int data_len)
接口描述	读事件信息接口
参数	“reader”:读事件指针，”data”: 获取事件的Buf，”data_len”：buf的大小
输出	无



## 参与贡献



\1.  Fork 本仓库

\2.  新建 Feat_xxx 分支

\3.  提交代码

\4.  新建 Pull Request



## 项目路线图

### 检测能力

| 特性                    | 发布时间 | 发布版本      |
| ----------------------- | -------- | ------------- |
| 检测框架                | 2023.10  | 22.03 LTS SP3 |
| 进程管理探针            | 2023.11  | 22.03 LTS SP3 |
| 文件操作探针            | 2023.11  | 22.03 LTS SP3 |
| 程序行为探针（API调用） | 2023.11  | 22.03 LTS SP3 |
| 内存数据探针（）        | 2023.11  | 22.03 LTS SP3 |
| 增加内核hook模式（）    | 2023.11  | 22.03 LTS SP3 |
| 资源消耗探针（）        | 2023.11  | 22.03 LTS SP3 |
| 账户管理探针            | 2024.03  |               |
| 设备操作探针            | 2024.03  |               |
| 网络访问探针            | 2024.06  |               |

### 响应能力

| 特性                        | 发布时间 | 发布版本      |
| --------------------------- | -------- | ------------- |
| 响应框架                    | 2023.10  | 22.03 LTS SP3 |
| 告警上报                    | 2023.10  | 22.03 LTS SP3 |
| 攻击阻断（恶意对象清理）    | 2023.11  | 22.03 LTS SP3 |
| 攻击阻断（内构实时阻断）    | 2023.11  | 22.03 LTS SP3 |
| 攻击阻断（恶意对象清理2.0） | 2024.03  |               |

### 服务能力

| 特性     | 发布时间 | 发布版本      |
| -------- | -------- | ------------- |
| 通信框架 | 2023.11  | 22.03 LTS SP3 |
| 订阅管理 | 2023.11  | 22.03 LTS SP3 |
| 日志转储 | 2023.11  | 22.03 LTS SP3 |
| 配置下发 | 2023.11  | 22.03 LTS SP3 |
| 即时检测 | 2023.11  | 22.03 LTS SP3 |



## 特技



\1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md

\2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)

\3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目

\4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目

\5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)

\6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
