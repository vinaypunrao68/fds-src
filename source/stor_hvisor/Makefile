# Makefile – makefile of our first driver
#  
# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
TOPDIR = ../include
EXTRA_CFLAGS += -I$(M)../include

ifneq (${KERNELRELEASE},)
	obj-m := fbd.o
# Otherwise we were called directly from the command line.
#Invoke the kernel build system.
else
#	KERNEL_SOURCE := /usr/src/linux-3.9.5
#	KERNEL_SOURCE := /usr/src/kernels/3.6.10-4.fc18.x86_64
	KERNEL_SOURCE := /usr/src/kernels/3.9.4-200.fc18.x86_64
	PWD := $(shell pwd)
default:
	${MAKE} -I$(TOPDIR) -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules
clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
endif

