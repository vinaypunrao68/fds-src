CC=gcc
TOPDIR=..
IDIR =-I$(TOPDIR)/include 
LDIR =-L$(TOPDIR)/lib -L. -L/usr/lib/mysql
CFLAGS=$(IDIR) -g -DDMGR_LIVE_DEBUG
LIBS=$(LDIR) -lpthread -lfds -lssl -lcrypto -lmysqlclient
SRCS = data_mgr.c unit_test.c
OBJS = $(SRCS:%.c=%.o)

DEPS = $(TOPDIR)/include/data_mgr.h $(TOPDIR)/include/fds_commons.h data_mgr_pvt.h

all: dmgrd unit_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dmgrd: data_mgr.o
	$(CC) -o $@ -g $< $(LIBS)

unit_test: unit_test.o
	$(CC) -o $@ -g $<

.PHONY: clean

clean:
	rm -f $(OBJS) *~ core
