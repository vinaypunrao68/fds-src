TOPDIR=..
IDIR =-I. -I$(TOPDIR)/include -I$(TOPDIR)/lib/leveldb-1.12.0/include/
CC=gcc
CPPC=g++
CFLAGS=$(IDIR) -g -L$(TOPDIR)/lib/leveldb-1.12.0/
LIBS=-lpthread $(TOPDIR)/lib/leveldb-1.12.0/libleveldb.a

DEPS = 

SRCS = stor_mgr.cc disk_mgr.cc

OBJS = $(SRCS:%.cc=%.o)

BINS = stor_mgr

all: stor_mgr 

%.o: %.cc $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

stor_mgr: stor_mgr.o disk_mgr.o 
	$(CPPC) -o $@ $(OBJS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) $(BINS) *~ core 
