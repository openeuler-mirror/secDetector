# 接口说明

secDetector操作系统内构入侵检测系统对外提供SDK，这里给出用户开发应用程序所需的接口。SDK的接口设计非常简单，一共只有三个接口，两个头文件。

头文件：

- secDetector/secDetector_sdk.h：包含接口定义
- secDetector/secDetector_topic.h：包含一些调用接口所需使用的预定义宏，如可选择性订阅的功能主题编号

## secSub

订阅 topic 接口。

**功能**：

订阅接口，应用程序通过输入不同 topic id，可以选择订阅不同的功能主题，比如文件打开类异常探针。secDetector 提供的诸功能主题对应的 topic id 的定义在 secDetector_topic.h 中可以查看。本订阅接口支持一次订阅多个主题，多个探针的 topic id 可以以位图的形式进行组合。

> [!NOTE]说明   
> 由于一次订阅产生一个reader即信息读取器，所以应用程序应当在一次订阅接口调用中订阅所需的所有主题。这样就可以使用一个reader进行信息的采集。如果需要调整订阅的内容，可以退订之后再重新订阅。

**函数声明：**

```c
void *secSub(const int topic);
```

**参数：**

- topic：入参，需要订阅的主题集合

**返回值：**

- NULL：订阅失败
- NOT NULL：读取订阅主题相关信息的GRPC reader读取器

## secUnsub

退订 topic 接口。

**功能**：

退订接口，应用程序通过输入订阅成功后获得的reader，完成主题的退订。退订后应用程序便不会收到相应主题的信息。系统中某主题如果没有任何应用程序订阅，则不会被执行。

**函数声明：**

```c
void secUnsub(void *reader);
```

**参数：**

- reader：入参，需要退订的信息读取器

**返回值：**

- 无

## secReadFrom

已订阅主题的消息读取接口。

**功能**：

使用订阅接口对某些主题的订阅成功后，执行退订操作之前，可使用本接口接受 secDetector 发送的已订阅主题的消息。本接口是阻塞式的。应用程序建议使用独立的线程循环调用。当已订阅主题有消息时候，本函数才会被恢复执行。

**函数声明：**

```c
void secReadFrom(void *reader, char *data, int data_len);
```

**参数：**

- reader：入参，主题订阅成功后得到的消息读取器
- data：出参，消息缓冲区，由应用程序提供的一段内存
- data_len：入参，消息缓冲区的尺寸

**返回值：**

- 无
