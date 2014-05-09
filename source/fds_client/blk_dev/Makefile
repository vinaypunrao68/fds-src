TOPDIR := $(PWD)/..
IDIR := -I$(TOPDIR)/include -I/usr/include
EXTRA_CFLAGS := -DLIB_KERNEL $(IDIR)
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

SRC := fbd_init.c ring.c  control.c request.c dev_ctrl.c

fbd-objs = $(SRC:.c=.o)

obj-m += fbd.o

all:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD}/blk_dev modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean
