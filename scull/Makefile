ifneq ($(KERNELRELEASE),)
	obj-m := scull.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	rm -f ./*.o ./*.ko ./*.mod.c ./*.mod.o ./*.mod ./modules.order ./Module.symvers ./.*.cmd
