TOPDIR=..
IDIR =-I$(TOPDIR)/include -I/usr/include/mysql
CC=gcc
CFLAGS=$(IDIR) -g -DTVC_DBG -DVVC_DBG
LDIR =-L/usr/lib/mysql
LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient

COMMON_DEPS = $(TOPDIR)/include/fds_commons.h 
TVC_DEPS = tvc_private.h $(TOPDIR)/include/tvclib.h
VVC_DEPS = vvc_private.h $(TOPDIR)/include/vvclib.h

TVC_SRCS = tvclib.c tvc_unit_test.c
VVC_SRCS = vvclib.c vvc_db.c vvc_unit_test.c 

TVC_OBJS = $(TVC_SRCS:%.c=%.o)
VVC_OBJS = $(VVC_SRCS:%.c=%.o)

ALL: tvc_unit_test vvc_unit_test

%.o: %.c $(COMMON_DEPS) $(TVC_DEPS) $(VVC_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tvc_unit_test: $(TVC_OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

vvc_unit_test: $(VVC_OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TVC_OBJS) $(VVC_OBJS) *~ core 
