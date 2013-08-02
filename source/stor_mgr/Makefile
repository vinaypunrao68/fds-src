TOPDIR=..
HASHDIR=$(TOPDIR)/util/hash
LEVELDBDIR=$(TOPDIR)/lib/leveldb-1.12.0/
IDIR =-I. -I$(TOPDIR) -I$(TOPDIR)/include/ -I$(LEVELDBDIR) -I$(LEVELDBDIR)/include/ -I$(HASHDIR)
CC=gcc
CPPC=g++
OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode
CFLAGS=$(IDIR) -L$(LEVELDBDIR) $(OPTS)
LIBS=-lpthread $(LEVELDBDIR)/libleveldb.a

DEPS = 

SRCS = stor_mgr.cpp disk_mgr.cpp odb.cpp

OBJS = $(SRCS:%.cpp=%.o)

TESTUTIL = \
	$(LEVELDBDIR)/util/testutil.o
HASHUTIL = \
	$(HASHDIR)/MurmurHash3.o

TESTS = \
	odb_unit_test

BINS = stor_mgr odb_unit_test

all: stor_mgr $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

stor_mgr: stor_mgr.o disk_mgr.o odb.o
	$(CPPC) -o $@ $(OBJS) $(LIBS)

odb_unit_test: odb_unit_test.cpp odb.o $(HASHUTIL) $(TESTUTIL)
	$(CPPC) -o $@ odb_unit_test.cpp odb.o $(HASHUTIL) $(TESTUTIL) $(LIBS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) $(BINS) $(TESTS)
