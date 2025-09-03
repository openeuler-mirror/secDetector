# Interface Description

secDetector provides an external SDK. This document outlines the interfaces required for users to develop applications. The SDK interface design is straightforward, comprising a total of three interfaces and two header files.

Header files:

- **secDetector/secDetector_sdk.h**: contains interface definitions

- **secDetector/secDetector_topic.h**: includes predefined macros necessary for calling interfaces, such as the topic numbers for optional subscription features

## secSub

Topic subscription interface

**Function**:

This is the subscription interface. Applications can subscribe to different functional topics by providing different topic IDs, for instance, abnormal probe events related to file opening. The definitions of the topic IDs for the various functional topics offered by secDetector are available in secDetector_topic.h. This interface supports subscribing to multiple topics simultaneously. The topic IDs of multiple probes can be combined using a bitmap.

> ![NOTE]NOTE
> Since each subscription creates a reader (an information reader), applications should subscribe to all necessary topics in a single call to the subscription interface. This allows for collecting information using a single reader. If you need to modify the subscribed content, you can unsubscribe and then resubscribe.

**Function declaration:**

```c
void *secSub(const int topic);
```

**Parameters:**

- `topic`: Input parameter, the set of topics to subscribe to

**Return Value:**

- Null: Subscription failed
- Not null: The GRPC reader for reading information related to the subscribed topics

## secUnsub

Topic unsubscription interface

**Function**:

This is the unsubscription interface. Applications can unsubscribe from topics by providing the reader obtained after a successful subscription. After unsubscribing, the application will no longer receive information on the corresponding topics. If no application is subscribed to a particular topic in the system, that topic will not be processed.

**Function Declaration:**

```c
void secUnsub(void *reader);
```

**Parameters:**

- `reader`: Input parameter, the information reader to unsubscribe

**Return Value:**

- None

## secReadFrom

Message reading interface for subscribed topics

**Function**:

After successfully subscribing to certain topics using the subscription interface, and before performing the unsubscription operation, this interface can be used to receive messages on the subscribed topics sent by secDetector. This interface is blocking. It is recommended that applications use a separate thread to call this function in a loop. This function will only resume execution when there is a message available for the subscribed topic.

**Function Declaration:**

```c
void secReadFrom(void *reader, char *data, int data_len);
```

**Parameters:**

- `reader`: Input parameter, the message reader obtained after successful topic subscription
- `data`: Output parameter, the message buffer, a block of memory provided by the application
- `data_len`: Input parameter, the size of the message buffer

**Return Value:**

- None
