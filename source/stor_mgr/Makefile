TOPDIR=..
IDIR =-I. -I$(TOPDIR)/include -I$(TOPDIR)/lib/leveldb-1.12.0/include/
CC=gcc
CPPC=g++
CFLAGS=$(IDIR) -g -L$(TOPDIR)/lib/leveldb-1.12.0/
LIBS=-lpthread $(TOPDIR)/lib/leveldb-1.12.0/libleveldb.a

DEPS = 

SRCS = stor_mgr.c disk_mgr.c

OBJS = $(SRCS:%.c=%.o)

BINS = stor_mgr

ALL: stor_mgr 

%.o: %.c $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

stor_mgr: stor_mgr.o disk_mgr.o 
	$(CPPC) -o $@ $(OBJS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) $(BINS) *~ core 
