TOPDIR  = ..
ROOTDIR = $(TOPDIR)
TOPICEDIR = ../lib/Ice-3.5.0/cpp
BOOSTIDIR = /usr/local/include/
BOOSTLDIR = /usr/local/lib/

IDIR = -I$(TOPDIR)/include -I$(TOPDIR)/fdsp -I$(BOOSTIDIR) -I$(TOPICEDIR)/include/Ice -I. -I$(TOPICEDIR)/include

#IDIR = -I. -I$(ROOTDIR) -I$(BOOSTIDIR) -I$(TOPICEDIR)/include/ice -I$(TOPICEDIR)/include -I$(TOPDIR)/fdsp -I../ -I$(TOPDIR)/include


CPPC = g++
OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CFLAGS = $(IDIR) $(OPTS)
CPP11	= -std=c++0x
DEPS = $(TOPDIR)/fdsp/FDSP.h

MY_FLAGS = -std=c++0x -Wall -Werror
FDS_OBJS   = ../fdsp/FDSP.o

BOOST_LIBS = -lboost_log -lboost_system -lboost_thread -lboost_date_time
MY_LIBS          = -L../lib/Ice-3.5.0/cpp/lib ../lib/leveldb-1.12.0/libleveldb.so -lboost_log -lboost_system -lboost_thread \
                  -lboost_date_time -lIce -lIceUtil -lIceStorm -lpthread -lssl -lcrypto
LIBS = $(BOOST_LIBS) -lpthread

CPPFLAGS	:= -I. $(CPPFLAGS) -I. -I$(TOPICEDIR)/include/Ice \
	-I../ \
	-I$(TOPDIR)/fdsp \
	-I$(TOPDIR)/include \
	-I/usr/local/include \
	$(CPP11)

SRCS = \
	Log.cpp PerfStat.cpp

OBJS = $(SRCS:%.cpp=%.o)

TESTS = \
	log_unit_test perfstat_unit_test

all: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) $(CPPFLAGS) -c -o $@ $< $(CFLAGS) $(MY_FLAGS)

log_unit_test: log_unit_test.cpp $(OBJS) $(FDS_OBJS)
	$(CPPC) -o $@ log_unit_test.cpp $(OBJS) $(LIBS) $(FDS_OBJS) $(CPPFLAGS) $(CFLAGS) $(MY_LIBS)

perfstat_unit_test: perfstat_unit_test.cpp $(OBJS) $(FDS_OBJS)
	$(CPPC) -o $@ perfstat_unit_test.cpp $(OBJS) $(LIBS) $(FDS_OBJS) $(CPPFLAGS) $(CFLAGS) $(MY_FLAGS) $(MY_LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS)
