TOPDIR      = ..
LEVELDB_DIR = ./leveldb-1.12.0/
ICEDIR = $(TOPDIR)/lib/Ice-3.5.0

LDIR = -L$(LEVELDB_DIR)
#IDIR = -I$(TOPDIR)/include -I/usr/include/mysql -I$(TOPDIR)
IDIR = -I$(TOPDIR) -I$(LEVELDB_DIR) -I$(LEVELDB_DIR)/include/ -I$(TOPDIR)/fdsp -I$(ICEDIR)/cpp/include

OPTS ?= -g2              # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CPPC	= g++

CPP11	= -std=c++0x

CFLAGS	= -Wall -Werror $(CPP11) $(IDIR) $(LDIR) $(OPTS)

#LDIR =-L/usr/lib/mysql -L/usr/lib64/mysql -L.
#LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient -lfds

LEVELDB_LIBS = -lleveldb
LIBS = $(LEVELDB_LIBS) -L$(ICEDIR)/cpp/lib -lIceStorm -lIce -lIceUtil -lpthread

LIBK_IDIR=-I$(TOPDIR)/include -I/usr/src/kernels/3.3.4-5.fc17.i686/include/ -I/usr/src/kernels/3.3.4-5.fc17.i686/arch/x86/include/
LIBK_CFLAGS=$(LIBK_IDIR) -g -DVVC_DBG -DLIB_KERNEL -D__KERNEL__ -DMODULE

#DEPS = tvc_private.h vvc_private.h$(TOPDIR)/include/tvclib.h
DEPS = \
	Catalog.h \
	VolumeCatalog.h 

#SRCS = vvc_private.c tvclib.c vvclib.c vvc_db.c
SRCS = \
	Catalog.cpp \
	VolumeCatalog.cpp \
	OMgrClient.cpp

#UTEST_SRCS = tvc_unit_test.c vvc_unit_test.c
TESTS = \
	catalog_unit_test \
	om_client_unit_test

OBJS = $(SRCS:%.cpp=%.o)
#UTEST_OBJS = $(UTEST_SRCS:%.cpp=%.o)
UTEST_OBJS = ../fdsp/fds_pubsub.o

# TODO: Clean this up
#all: libfds.a tvc_unit_test vvc_unit_test
all: libfds.a 

utest: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

catalog_unit_test: catalog_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ catalog_unit_test.cpp $(OBJS) $(UTEST_OBJS) $(LIBS) $(CFLAGS)
om_client_unit_test: om_client_unit_test.cpp libfds.a
	$(CPPC) -o $@ om_client_unit_test.cpp -L. -lfds $(UTEST_OBJS) $(LIBS) $(CFLAGS)

libfds.a: $(OBJS)
	ar -rcs $@ $(OBJS)

tvc_unit_test: tvc_unit_test.o libfds.a
	$(CC) -o $@ -g tvc_unit_test.o $(LIBS)

vvc_unit_test: vvc_unit_test.o libfds.a
	$(CC) -o $@ -g vvc_unit_test.o $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS) libfds.a *~ core 
