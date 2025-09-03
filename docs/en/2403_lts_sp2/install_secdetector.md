# Installing secDetector

## Hardware and Software Requirements

### Hardware Requirements

- Currently supports only x86_64 and aarch64 architecture processors.
- secDetector disk usage requirement: 1GB or more quota.
- secDetector memory usage requirement: 100MB or more quota.

### Environment Preparation

Install openEuler by referring to [Installation Guide](https://docs.openeuler.openatom.cn/en/docs/24.03_LTS_SP2/server/installation_upgrade/installation/installation_on_servers.html).

## Installing secDetector

1. Configure the openEuler Yum repository: The Yum repository is configured by default in openEuler, so no additional operations are required. In special cases, refer to the official openEuler documentation to configure an online Yum repository or configure a local Yum repository by mounting the ISO.

2. Install secDetector.

   ```shell
   #Install secDetector
   sudo yum install secDetector
   ```

> ![NOTE]NOTE
>
> After installing secDetector, the relevant files required for deploying secDetector can be found in the specified directories:

```shell
# Core framework of secDetector kernel driver
/lib/modules/%{kernel_version}/extra/secDetector/secDetector_core.ko

# Functional components of secDetector kernel driver
/lib/modules/%{kernel_version}/extra/secDetector/secDetector_xxx.ko

# Daemon process file of secDetector
/usr/bin/secDetectord

# SDK library files of secDetector
/usr/lib64/secDetector/libsecDetectorsdk.so
/usr/include/secDetector/secDetector_sdk.h
/usr/include/secDetector/secDetector_topic.h
```

## Deploying secDetector

The main component of secDetector, secDetectord, is deployed in the system as a system service. The foreground service system can communicate with it by integrating the SDK. Since some capabilities of secDetector must be built within the kernel, the full set of secDetectord functions also depends on the deployment of its backend driver.

### Deploying the Kernel Driver

1. Insert the basic framework of the kernel driver. **secDetector_core.ko** is the basic framework of the kernel driver and should be deployed before other kernel modules. Find the directory of the installed secDetector_core.ko and insert it into the kernel. Refer to the following command:

   ```shell
   sudo insmod secDetector_core.ko
   ```

   `secDetector_core` supports one parameter, `ringbuf_size`. You can control the size of the cache space for the data channel between the kernel driver and the user-space secDetectord by specifying the value of this parameter. This parameter can be specified as an integer between 4 and 1024, in units of MB. The default value is 4 and must be a power of 2. Refer to the following command:

   ```shell
   sudo insmod secDetector_core.ko ringbuf_size=128
   ```

2. Insert the functional modules of the kernel driver. The kernel driver of secDetector adopts a modular deployment method. You can choose to deploy functional modules that meet their needs based on the framework, or they can choose to deploy all modules. Refer to the following commands:

   ```shell
   sudo insmod secDetector_kmodule_baseline.ko

   sudo insmod secDetector_memory_corruption.ko

   sudo insmod secDetector_program_action.ko

   sudo insmod secDetector_xxx.ko
   ```

   - **secDetector_kmodule_baseline.ko** provides the ability to detect kernel module lists and belongs to the memory modification probe category.
   - **secDetector_memory_corruption.ko** provides the ability to detect memory corruption and belongs to the memory modification probe category.
   - **secDetector_program_action.ko** provides the ability to detect program behavior and belongs to the program behavior probe category.

### Deploying the User-Space Driver and observer_agent

Currently, the user-space driver and the service observer_agent have been integrated into secDetectord. Refer to the following command:

```shell
sudo ./secDetectord &
```

The usr-space driver includes the capabilities of file operation probes and process management probes.

secDetectord supports the following configuration options:

```text
Usage: secDetectord [options]
secDetectord runs in the background by default, obtaining data from probes and forwarding it to subscribers.
Options:
  -d         Enter debug mode, run in the foreground, and print probe data to the console.
  -s <size>  Configure the eBPF buffer size in Mb, default is 4; the size range is 4~1024, and it must be a power of 2. Currently, there are 2 independent buffers.
  -t <topic> Supports configuring subscribed events, default is all events. topic is in bitmap format. For example, -t 0x60 subscribes to both process creation and process exit events. For details, please refer to include/secDetector_topic.h.
```

### Deploying the SDK

The SDK library files are deployed in the system library directory by default. Users only need to include the SDK header files in their programs to use it.
