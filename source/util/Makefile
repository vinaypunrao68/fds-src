TOPDIR  = ..
ROOTDIR = $(TOPDIR)
BOOSTIDIR = /usr/local/include/
BOOSTLDIR = /usr/local/lib/
IDIR = -I. -I$(ROOTDIR) -I$(BOOSTIDIR)

CPPC = g++
OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CFLAGS = $(IDIR) $(OPTS) -D_REENTRANT

BOOST_LIBS = -lboost_log -lboost_system -lboost_thread -lboost_date_time
LIBS = $(BOOST_LIBS) -lpthread

SRCS = \
	Log.cpp PerfStat.cpp

OBJS = $(SRCS:%.cpp=%.o)

TESTS = \
	log_unit_test perfstat_unit_test

all: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

log_unit_test: log_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ log_unit_test.cpp $(OBJS) $(LIBS) $(CFLAGS)

perfstat_unit_test: perfstat_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ perfstat_unit_test.cpp $(OBJS) $(LIBS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS)
