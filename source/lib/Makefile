TOPDIR      = ..
LEVELDB_DIR = ./leveldb-1.12.0/

LDIR = -L$(LEVELDB_DIR)
#IDIR = -I$(TOPDIR)/include -I/usr/include/mysql -I$(TOPDIR)
IDIR = -I$(TOPDIR) -I$(LEVELDB_DIR) -I$(LEVELDB_DIR)/include/

OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CPPC   = g++
CFLAGS = -Wall -Werror $(IDIR) $(LDIR) $(OPTS)
#LDIR =-L/usr/lib/mysql -L/usr/lib64/mysql -L.
#LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient -lfds

LEVELDB_LIBS = -lleveldb
LIBS = $(LEVELDB_LIBS)

LIBK_IDIR=-I$(TOPDIR)/include -I/usr/src/kernels/3.3.4-5.fc17.i686/include/ -I/usr/src/kernels/3.3.4-5.fc17.i686/arch/x86/include/
LIBK_CFLAGS=$(LIBK_IDIR) -g -DVVC_DBG -DLIB_KERNEL -D__KERNEL__ -DMODULE

#DEPS = tvc_private.h vvc_private.h$(TOPDIR)/include/tvclib.h
DEPS = \
	Catalog.h \
	VolumeCatalog.h

#SRCS = vvc_private.c tvclib.c vvclib.c vvc_db.c
SRCS = \
	Catalog.cpp \
	VolumeCatalog.cpp

#UTEST_SRCS = tvc_unit_test.c vvc_unit_test.c
TESTS = \
	catalog_unit_test

OBJS = $(SRCS:%.cpp=%.o)
#UTEST_OBJS = $(UTEST_SRCS:%.cpp=%.o)

# TODO: Clean this up
#all: libfds.a tvc_unit_test vvc_unit_test
all: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

catalog_unit_test: catalog_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ catalog_unit_test.cpp $(OBJS) $(LIBS) $(CFLAGS)

libfds.a: $(OBJS)
	ar -rcs $@ $(OBJS)

tvc_unit_test: tvc_unit_test.o libfds.a
	$(CC) -o $@ -g tvc_unit_test.o $(LIBS)

vvc_unit_test: vvc_unit_test.o libfds.a
	$(CC) -o $@ -g vvc_unit_test.o $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) *~ core 
