# make secDetector_core

obj-m += secDetector_memory_corruption.o
#obj-m += secDetector_task_block.o
#obj-m += secDetector_file_block.o
obj-m += secDetector_kmodule_baseline.o
# obj-m += secDetector_kprobe_example.o
# obj-m += secDetector_lsm_example.o
obj-m += secDetector_program_action.o

secDetector_memory_corruption-objs := memory_corruption/secDetector_memory_corruption.o memory_corruption/secDetector_mc_kmodule_list.o
#secDetector_task_block-objs := task_block/secDetector_task_block.o
#secDetector_file_block-objs := file_block/secDetector_file_block.o
secDetector_kmodule_baseline-objs := kmodule_baseline/secDetector_kmodule_baseline.o kmodule_baseline/secDetector_mc_kmodule_baseline.o
# secDetector_kprobe_example-objs := kprobe_example/secDetector_kprobe_example.o
# secDetector_lsm_example-objs := lsm_example/secDetector_lsm_example.o
secDetector_program_action-objs := program_action/secDetector_program_action.o

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

cflags-y += -I$(PWD)/../include
cflags-y += -I$(KERNEL_SRC)/include/linux
EXTRA_CFLAGS += -Wall -Werror
ifndef KDIR
KDIR=$(KERNEL_SRC)
endif

KBUILD_EXTRA_SYMBOLS += $(PWD)/../core/Module.symvers
export KBUILD_EXTRA_SYMBOLS

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules KCPPFLAGS="${cflags-y}"

clean:
	rm -rf *.ko *.o .*.o.d .*.ko.cmd *.mod.c *.mod .*.cmd Modules.* *.symvers *.order .tmp* *.markers */*.ko */*.o */.*.o.d */.*.ko.cmd */*.mod.c */*.mod */.*.cmd */Modules.* */*.symvers
