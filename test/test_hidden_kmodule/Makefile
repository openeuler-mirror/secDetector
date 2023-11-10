obj-m := secDetector_test_hidden_kmodule.o
secDetector_test_hidden_kmodule-objs:=test_hidden_kmodule.o hide_kmodule.o hide_utils.o

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

cflags-y += -I$(KERNEL_SRC)/include/linux

default:
	make -C $(KERNEL_SRC) M=`pwd` modules KCPPFLAGS="${cflags-y}"
clean:
	rm -rf *.ko *.o *.ko.cmd *.mod.c .*.o.cmd *.o Modules.* *.symvers *.order .tmp* ,markers .*.cmd *.mod
