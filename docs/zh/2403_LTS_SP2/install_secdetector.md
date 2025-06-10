# 安装 secDetector

## 软硬件要求

### 硬件要求

* 当前仅支持 x86_64、aarch64 架构处理器。
* secDetector磁盘使用需求：配额1GB及以上。
* secDetector内存使用需求：配额100MB及以上。

### 环境准备

安装 openEuler 系统，安装方法参考《[安装指南](https://docs.openeuler.org/zh/docs/24.03_LTS/docs/Installation/installation.html)》。

## 安装secDetector

1. 配置openEuler yum源：openEuler 发布版本上已默认配置完成yum源，无需额外操作。特殊情况下请参考openEuler官方文档配置在线yum源或通过ISO挂载配置本地yum源。

2. 安装secDetector。

   ```shell
   #安装secDetector
   sudo yum install secDetector
   ```

> ![](./public_sys-resources/icon-note.gif)说明：
>
> 安装secDetector后在指定目录下可获得部署secDetector所需的相关文件：

```shell
#secDetector的kerneldriver的核心框架
/lib/modules/%{kernel_version}/extra/secDetector/secDetector_core.ko

#secDetector的kerneldriver的功能组件
/lib/modules/%{kernel_version}/extra/secDetector/secDetector_xxx.ko

#secDetector的守护者进程文件
/usr/bin/secDetectord

#secDetector的SDK库文件
/usr/lib64/secDetector/libsecDetectorsdk.so
/usr/include/secDetector/secDetector_sdk.h
/usr/include/secDetector/secDetector_topic.h
```

## 部署 secDetector

secDetector的主体secDetectord是以系统服务的形式部署在系统中的，前台业务系统可以通过集成SDK来与之通信。由于secDetector的部分能力必须构建在内核之中，因此secDetectord的功能全集还依赖于其后台驱动的部署。

### 部署 kernel driver

1. 插入 kernel driver 的基础框架：secDetector_core.ko 是 kernel driver 的基础框架，要优先于其他内核模块进行部署。找到安装后的 secDetector_core.ko 目录，将其插入内核。参考命令如下：

   ```shell
   sudo insmod secDetector_core.ko
   ```

   secDetector_core 支持一个命令行参数ringbuf_size。用户可以通过指定该参数的值来控制 kernel driver 与 用户态secDetectord之间数据通道的缓存空间尺寸。该参数可以被指定为4~1024中的一个整数，单位是MB。默认值是4，必须为2的幂。参考命令如下：

   ```shell
   sudo insmod secDetector_core.ko ringbuf_size=128
   ```

2. 插入 kernel driver 的功能模块：secDetector的 kernel driver 采用模块化部署方式。用户可以选择基于框架部署满足需要的功能模块，也可以选择部署全部模块。参考命令如下：

   ```shell
   sudo insmod secDetector_kmodule_baseline.ko
   
   sudo insmod secDetector_memory_corruption.ko
   
   sudo insmod secDetector_program_action.ko
   
   sudo insmod secDetector_xxx.ko
   ```

   - secDetector_kmodule_baseline.ko 提供了内核模块列表检测的能力，属于内存修改类探针；
   - secDetector_memory_corruption.ko 提供了内存修改检测的能力，属于内存修改类探针；
   - secDetector_program_action.ko 提供了程序行为检测的能力，属于程序行为类探针。

### 部署 usr driver 和 observer_agent

当前用户态驱动 usr driver 和服务 observer_agent 已经都被集成到secDetectord中，参考命令如下：

```shell
sudo ./secDetectord &
```

usr driver当前包含了文件操作类探针和进程管理类探针的能力。

secDetectord支持如下一些配置选项：

```text
用法：secDetectord [选项]
secDetectord 默认会在后台运行，从探针中取得数据并转发给订阅者。
选项：
  -d         进入调试模式，进入前台运行，并且在控制台打印探针数据。
  -s <size>  配置eBPF缓冲区大小，单位为Mb，默认为4； size可选范围为4~1024，且必须为2的幂次方。当前拥有2个独立的缓冲区。
  -t <topic> 支持配置订阅的事件，默认为所有事件。topic 是位图格式。例如 -t 0x60 同时订阅进程创建和进程退出事件。详细请查阅 include/secDetector_topic.h。
```

### 部署SDK

SDK的库文件默认已经被部署到系统库目录中，用户需要在自己的程序中引用SDK的头文件即可使用。
