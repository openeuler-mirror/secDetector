# Introduction to secDetector

## Introduction

secDetector is an OS-native intrusion detection system designed to protect critical information infrastructure with robust detection and response capabilities. It aims to reduce development costs for third-party security tools while enhancing their detection and response capabilities. secDetector models security event primitives based on the ATT&CK attack pattern library, offering richer primitives and providing real-time blocking and flexible response capabilities.

As a flexible OS-native intrusion detection system, secDetector can be used in three modes:

1. Enabled directly by system users for basic anomaly event alerting and handling.
2. Integrated by security posture awareness services to compensate for system information collection deficiencies, used for complex security threat analysis like APT and real-time blocking for key event control.
3. Secondarily developed by security professionals or security awareness service providers to build accurate, efficient, and timely intrusion detection and response capabilities based on an extensible framework.

## Software Architecture

```text
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
||                                core                                    ||
||  -------------  ----------------  ----------------  -----------------  ||
||  | hook unit |  | collect unit |  | analyze unit |  | response unit |  ||
||  -------------  ----------------  ----------------  -----------------  ||
||                                                                        ||
||========================================================================||
```

secDetector is architecturally divided into four parts: SDK, service, detection feature set cases, and detection framework core.

- SDK

  The SDK is implemented as a user-space dynamic link library and is deployed in security awareness services that need to use secDetector. The SDK is used to communicate with the service of secDetector to perform required tasks (such as subscribing, unsubscribing, reading existing messages, etc.). The exception information provided by secDetector is defined as different cases, which security awareness services can subscribe to based on their needs.

- Service

  The service is implemented as a user-space service application. It manages and maintains the case subscription information for security awareness services and maintains the running status of cases. Information collected by the framework core and the detection feature set cases is uniformly collected by the service and forwarded to different security awareness services as needed. The configuration and management requirements of security awareness services for the underlying detection feature set cases and the framework core are also uniformly managed and forwarded by the service. Different security awareness services may require the same case. The service will calculate the union of cases required by all security awareness services and register it with the lower layer.

- Feature set cases

  The detection feature set cases are a series of exception detection probes. They take different forms depending on the exception information; for example, each probe for kernel exception information detection is implemented as a kernel module. A case represents a probe, and a probe often provides information about a type of exception or a type of exception event. For instance, a process probe focuses on event information such as process creation, termination, and attribute modification, while a memory modification probe collects information like kernel module lists and security switch statuses. Therefore, a probe may include monitoring multiple events, and the logic for monitoring these different events might not be deployable within the same execution flow. We use the concept of a workflow to represent the work of a probe within the same execution flow; a probe can contain one or more workflows. For example, for a process probe, process creation detection and process attribute modification detection are different workflows.

- Framework core

  The detection framework core is the basic framework that every case relies on, providing management for cases and common basic functional units required by workflows. The kernel exception information detection framework is implemented as a kernel module. A detection feature case can register itself with the framework or unregister from the framework. The framework can also provide specific interaction interfaces to meet external dynamic requests. A workflow is defined as being composed of four types of functional units: event generator, information collector, event analyzer, and response unit.

The feature set cases and the framework core together are referred to as the driver. The driver provides the lowest-level system-wide implementation of secDetector functions.

Drivers are divided into two categories: kerneldriver and usrdriver. As the names suggest, the kerneldriver is deployed in kernel space and implemented as a kernel module. The usrdriver is deployed in user space and is directly deployed as a module within the service. Logically, the usrdriver is below the service, but in operation, to reduce communication costs, the usrdriver is directly integrated into the service program.

## Capabilities and Features

### Detection Capabilities

| Feature                            | Status     | Description                                                                 |
| ---------------------------------- | ---------- | --------------------------------------------------------------------------- |
| Detection Framework                | Implemented | A unified, flexible, extensible, and efficient detection framework supporting different types of trigger, collection, analysis, and response units. |
| Process Management Probes          | Implemented | Monitors events such as process creation, termination, and metadata modification. |
| File Operation Probes              | Implemented | Monitors events such as file creation, deletion, read/write, and attribute modification. |
| Program Behavior Probes (API Calls) | Implemented | Monitors key program behaviors such as anonymous pipe creation, command execution, and ptrace system calls. |
| Memory Modification Probes (Key Kernel Data) | Implemented | Monitors key kernel data such as kernel module lists and hardware security feature switches. |

### Response Capabilities

| Feature        | Status     | Description                                     |
| -------------- | ---------- | ----------------------------------------------- |
| Response Framework | Implemented | A unified, flexible, and extensible response framework supporting different types of response units. |
| Alert Reporting | Implemented | A response unit providing the capability to report exception information. |

### Service Capabilities

| Feature          | Status     | Description                                                                                                                                |
| ---------------- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| Communication Framework | Implemented | Applications communicate with the service using gRPC. Functionality is encapsulated in the SDK dynamic library.                         |
| Subscription Management | Implemented | Applications can subscribe once and use secDetector to obtain information long term. secDetector manages the subscribed applications and distributes information for the corresponding subscribed topics. |
| Configuration Delivery | Implemented | The service can configure specific detection and blocking features via parameters to achieve filtering, adjustment, and other functions. Currently not open to applications. |
| Real-time Detection | Implemented | The information provided by secDetector is real-time, accurate, and firsthand.                                                        |
