TOPDIR  = ..
ROOTDIR = ../..
BOOSTIDIR = /usr/include/boost/
BOOSTLDIR = /usr/lib64/
IDIR = -I. -I$(ROOTDIR) -I$(BOOSTIDIR)

CPPC = g++
OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CFLAGS = $(IDIR) $(OPTS) -D_REENTRANT

BOOST_LIB = -lboost_thread-mt
LIBS = $(BOOST_LIB) -lpthread

SRCS = \
	Mutex.cpp \
	Synchronization.cpp \
	ThreadPool.cpp

OBJS = $(SRCS:%.cpp=%.o)

TESTS = \
	conc_unit_test

all: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

conc_unit_test: conc_unit_test.cpp $(OBJS)
	$(CPPC) -o $@ conc_unit_test.cpp $(OBJS) $(LIBS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS)
