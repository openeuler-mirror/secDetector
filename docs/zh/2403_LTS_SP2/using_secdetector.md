# 使用 secDetector

secDetector 提供了SDK，一个so库，用户可以在自己的应用程序中集成该动态链接库从而通过简单的接口使用secDetector。本章介绍其使用方法。

## 基本用法

用户按照指南《[安装secDetector](./install_secdetector.md)》安装完secDetector之后，libsecDetectorsdk.so、secDetector_sdk.h、secDetector_topic.h就已经被部署到系统用户库默认路径中。

1. 使用 C 或 C++ 开发的应用程序确保include路径包含后，可以首先在程序中引用这两个头文件。

   ```c
   #include <secDetector/secDetector_topic.h>
   #include <secDetector/secDetector_sdk.h>
   ```

2. 参考指南《[接口参考](./api_reference.md)》调用SDK提供的接口访问secDetector。

   1. 首先调用订阅接口secSub，订阅所需的主题。
   2. 然后在独立线程中调用消息读取接口secReadFrom阻塞式的读取被订阅主题产生的信息。
   3. 最后当不需要使用secDetector时，调用退订接口secUnsub。退订时请严格使用订阅时的返回值。

## 代码示例

可以参考secDetector代码仓上的示例代码，由python语言编写。

1. 可以在如下链接中查看示例代码

   [examples/python · openEuler/secDetector (gitee.com)](https://gitee.com/openeuler/secDetector/tree/master/examples/python)

2. 也可以下载后参考

```shell
git clone https://gitee.com/openeuler/secDetector.git
```

## 规格与约束

1. 部分功能（如内存修改探针-安全开关）依赖硬件体系结构，因此在不同指令集架构上的表现并不相同。
2. 从内核到用户态传输数据缓存空间为探针共享，缓冲区满会丢弃新采集的事件信息。缓存空间可配置范围为4~1024 MB, 必须为2的幂。
3. 服务进程secDetectord支持root用户运行，不支持多实例，非第一个运行的程序会退出。
4. 用户订阅连接数限制为5个。
5. 用户订阅后，读取消息时需要为消息读取接口提供一块缓冲区，超过缓冲区长度的消息将被截断。建议缓冲区长度不低于4096。
6. 对于文件名、节点名之类的描述字符串都有一定的长度限制，过长可能会被截断。
7. 应用程序单进程内不支持并行多连接 secDetectord 接收消息。只能一次订阅，单连接接受消息。去订阅后才能重新订阅。
8. secDetectord 进程应当等待所有应用程序的连接中断即完全退订所有主题后，才可以关闭退出。
9. 部分功能（如内存修改探针-安全开关）基于当前CPU状态。因此检测的基本功能是可以检测到当前CPU上的状态变化，其他CPU上的状态变化如果未能及时同步到当前CPU，则不会被检测到。
