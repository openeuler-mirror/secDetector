# make secDetector_core

obj-m += secDetector_memory_corruption.o

secDetector_memory_corruption-objs := memory_corruption/secDetector_memory_corruption.o memory_corruption/secDetector_mc_kmodule_list.o



KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

cflags-y += -I$(PWD)/../include
cflags-y += -I$(KERNEL_SRC)/include/linux
EXTRA_CFLAGS += -Wall -Werror
ifndef KDIR
KDIR=$(KERNEL_SRC)
endif

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules KCPPFLAGS="${cflags-y}"

clean:
	rm -rf *.ko *.o .*.o.d .*.ko.cmd *.mod.c *.mod .*.cmd Modules.* *.symvers *.order .tmp* *.markers */*.ko */*.o */.*.o.d */.*.ko.cmd */*.mod.c */*.mod */.*.cmd */Modules.* */*.symvers
