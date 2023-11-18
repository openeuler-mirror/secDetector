# make secDetector_core

obj-m := secDetector_core.o

secDetector_core-objs += hook_unit/secDetector_hook_tracepoint.o
secDetector_core-objs += hook_unit/secDetector_hook_kprobe.o
secDetector_core-objs += hook_unit/secDetector_hook.o
secDetector_core-objs += collect_unit/secDetector_time.o
secDetector_core-objs += collect_unit/secDetector_function_switch.o
secDetector_core-objs += collect_unit/secDetector_collect.o
secDetector_core-objs += analyze_unit/secDetector_analyze.o
secDetector_core-objs += analyze_unit/secDetector_save_check.o
secDetector_core-objs += response_unit/secDetector_proc.o
secDetector_core-objs += response_unit/secDetector_response.o
secDetector_core-objs += response_unit/secDetector_ringbuffer.o
secDetector_core-objs += secDetector_workflow.o
secDetector_core-objs += secDetector_manager.o
secDetector_core-objs += secDetector_main.o

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

cflags-y += -I$(PWD)/../include
cflags-y += -I$(KERNEL_SRC)/include/linux
//EXTRA_CFLAGS += -Wall -Werror
EXTRA_CFLAGS += -Wall
ifndef KDIR
KDIR=$(KERNEL_SRC)
endif
export KBUILD_EXTRA_SYMBOLS

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules KCPPFLAGS="${cflags-y}"

clean:
	rm -rf *.ko *.o .*.o.d .*.ko.cmd *.mod.c *.mod .*.cmd Modules.* *.symvers *.order .tmp* *.markers */*.ko */*.o */.*.o.d */.*.ko.cmd */*.mod.c */*.mod */.*.cmd */Modules.* */*.symvers
