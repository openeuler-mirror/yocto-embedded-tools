KERNELDIR := ${DL_SDK_KERNELDIR}
CURRENT_PATH := $(shell pwd)

target := cpu_handler_dev
obj-m := $(target).o

build := kernel_modules

kernel_modules:
		$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
		$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
