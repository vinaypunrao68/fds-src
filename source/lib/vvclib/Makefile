TOPDIR=../..
IDIR =-I$(TOPDIR)/include -I/usr/include/mysql
CC=gcc
CFLAGS=$(IDIR) -g -DVVC_DBG
LDIR =-L/usr/lib/mysql
LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient

DEPS = vvc_private.h ../include/vvclib.h ../include/fds_commons.h

SRCS = unit_test.c vvclib.c vvc_db.c

OBJS = $(SRCS:%.c=%.o)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

unit_test: $(OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) *~ core 
