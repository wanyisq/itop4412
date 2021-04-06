#!/bin/bash
export ARCH=arm
obj-m += bmp085.o 

KDIR := /home/w/workspace/itop4412/itop_4_14_20200403/linux-4.14.2_iTop-4412_scp
CROSS_COMPILE = /home/w/workspace/itop4412/fsl-linaro-toolchain/bin/arm-none-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
TARGE = bmp_app

PWD ?= $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc -o $(TARGE) $(TARGE).c
	cp *.ko ~/nfs/
	cp $(TARGE) ~/nfs/
		
clean:
	rm *.ko *.o  *.mod.c *.order *.symvers
	rm -r $(TARGE)
	rm -rf .*
