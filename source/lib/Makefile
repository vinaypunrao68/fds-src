TOPDIR=..
IDIR =-I$(TOPDIR)/include -I/usr/include/mysql
CC=gcc
CFLAGS=$(IDIR) -g -DTVC_DBG -DVVC_DBG
LDIR =-L/usr/lib/mysql -L.
LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient -lfds

LIBK_IDIR=-I$(TOPDIR)/include -I/usr/src/kernels/3.3.4-5.fc17.i686/include/ -I/usr/src/kernels/3.3.4-5.fc17.i686/arch/x86/include/
LIBK_CFLAGS=$(LIBK_IDIR) -g -DVVC_DBG -DLIB_KERNEL -D__KERNEL__ -DMODULE

DEPS = tvc_private.h vvc_private.h$(TOPDIR)/include/tvclib.h

SRCS = tvclib.c vvclib.c vvc_db.c
UTEST_SRCS = tvc_unit_test.c vvc_unit_test.c

OBJS = $(SRCS:%.c=%.o)
UTEST_OBJS = $(UTEST_SRCS:%.c=%.o)

ALL: libfds.a tvc_unit_test vvc_unit_test
klib: libfds_k.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

libfds_k.o: vvclib.c
	$(CC) -c -o $@ $< $(LIBK_CFLAGS)

libfds.a: $(OBJS)
	ar -rcs $@ $(OBJS)

tvc_unit_test: tvc_unit_test.o libfds.a
	$(CC) -o $@ -g tvc_unit_test.o $(LIBS)

vvc_unit_test: vvc_unit_test.o libfds.a
	$(CC) -o $@ -g vvc_unit_test.o $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) *~ core 
