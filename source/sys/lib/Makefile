TOPDIR := $(PWD)/../..
IDIR := -I$(TOPDIR)/include -I$(TOPDIR)/sys/include -I/usr/include
EXTRA_CFLAGS := -DLIB_KERNEL $(IDIR)

SRC := vvclib.c vvc_db.c

libfdsk-objs = $(SRC:.c=.o)

obj-m += libfdsk.o

all:
	ln -s ../../lib/vvc_private.h vvc_private.h
	ln -s ../../lib/vvc_db.c vvc_db.c
	ln -s ../../lib/vvclib.c vvclib.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm *~ vvc_private.h vvc_db.c vvclib.c
