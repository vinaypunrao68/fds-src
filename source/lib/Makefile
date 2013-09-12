TOPDIR      = ..
LEVELDB_DIR = ./leveldb-1.12.0/
ICEDIR = $(TOPDIR)/lib/Ice-3.5.0

LDIR = -L$(LEVELDB_DIR)
#IDIR = -I$(TOPDIR)/include -I$(TOPDIR)
#IDIR = -I$(TOPDIR) -I$(TOPDIR)/include/ -I$(LEVELDB_DIR) -I$(LEVELDB_DIR)/include/
IDIR = -I$(TOPDIR) -I$(LEVELDB_DIR) -I$(LEVELDB_DIR)/include/ -I$(TOPDIR)/fdsp -I$(ICEDIR)/cpp/include

UTIL_DIR   = ../util/
UTIL_OBJS  = $(UTIL_DIR)/Log.o

OPTS ?= -g2              # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CPPC	= g++

CPP11	= -std=c++0x

CFLAGS	= -Wall -Werror $(CPP11) $(IDIR) $(LDIR) $(OPTS)


LEVELDB_LIBS = -lleveldb
LIBS = $(LEVELDB_LIBS) -L$(ICEDIR)/cpp/lib -lIceStorm -lIce -lIceUtil -lpthread -lboost_log -lboost_system -lboost_thread -lboost_date_time

LIBK_IDIR=-I$(TOPDIR)/include -I/usr/src/kernels/3.3.4-5.fc17.i686/include/ -I/usr/src/kernels/3.3.4-5.fc17.i686/arch/x86/include/
LIBK_CFLAGS=$(LIBK_IDIR) -g -DVVC_DBG -DLIB_KERNEL -D__KERNEL__ -DMODULE

DEPS = \
	Catalog.h \
	VolumeCatalog.h \
	OMgrClient.h

SRCS = \
	Catalog.cpp \
	VolumeCatalog.cpp \
	OMgrClient.cpp

TESTS = \
	catalog_unit_test \
	om_client_unit_test

OBJS = $(SRCS:%.cpp=%.o)
UTEST_OBJS = ../fdsp/fdsp_types.o ../fdsp/fds_pubsub.o ../fdsp/FDSP.o $(UTIL_OBJS)

COFLAGS = -Wall -Werror $(CPP11) $(IDIR) $(LDIR) $(OPTS)

# TODO: Clean this up
all: $(TESTS) libfds.a

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

vvc_unit_test.o: vvc_unit_test.c
	$(CPPC) -c -o $@ $< $(COFLAGS)

catalog_unit_test: catalog_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ catalog_unit_test.cpp $(OBJS) $(UTEST_OBJS) $(LIBS) $(CFLAGS)
om_client_unit_test: om_client_unit_test.cpp libfds.a
	$(CPPC) -o $@ om_client_unit_test.cpp -L. -lfds $(UTEST_OBJS) $(LIBS) $(CFLAGS)

libfds.a:
	ar -rcs $@ $(OBJS)

#tvc_unit_test: tvc_unit_test.o libfds.a
#	$(CC) -o $@ -g tvc_unit_test.o $(LIBS)

vvc_unit_test: vvc_unit_test.o libfds.a
	$(CC) -o $@ -g vvc_unit_test.o $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS) libfds.a *~ core 
