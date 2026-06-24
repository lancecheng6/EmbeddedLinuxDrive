KERNELDIR ?= /opt/kernel_factory
CURRENT_PATH := $(shell pwd)
CROSS_COMPILE ?= /opt/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
ARCH ?= arm

obj-m := ap3216c_regmap.o

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		-C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

app:
	$(CROSS_COMPILE)gcc -o ap3216cApp ap3216cApp.c
