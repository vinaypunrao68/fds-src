TOPDIR := $(PWD)/..
IDIR := -I$(TOPDIR)/include -I/usr/include
EXTRA_CFLAGS := -DLIB_KERNEL $(IDIR)
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

SRC := fbd_init.c fbd_hash.c

fbd-objs = $(SRC:.c=.o)
fbd-objs += ../sys/lib/libfdsk.o

obj-m += fbd.o

all:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean
