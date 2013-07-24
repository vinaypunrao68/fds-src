TOPDIR=..
IDIR =-I. -I.. -I$(TOPDIR)/include -I$(TOPDIR)/lib/leveldb-1.12.0/ -I$(TOPDIR)/lib/leveldb-1.12.0/include/
CC=gcc
CPPC=g++
CFLAGS=$(IDIR) -g -L$(TOPDIR)/lib/leveldb-1.12.0/
LIBS=-lpthread $(TOPDIR)/lib/leveldb-1.12.0/libleveldb.a

DEPS = 

SRCS = stor_mgr.cpp disk_mgr.cpp odb.cpp

OBJS = $(SRCS:%.cpp=%.o)

TESTUTIL = ../lib/leveldb-1.12.0/util/testutil.o

TESTS = \
	odb_unit_test

BINS = stor_mgr odb_unit_test

all: stor_mgr $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

stor_mgr: stor_mgr.o disk_mgr.o odb.o
	$(CPPC) -o $@ $(OBJS) $(LIBS)

odb_unit_test: odb_unit_test.cpp odb.o
	$(CPPC) -o $@ odb_unit_test.cpp odb.o $(TESTUTIL) $(LIBS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) $(BINS) $(TESTS) *~ core 
