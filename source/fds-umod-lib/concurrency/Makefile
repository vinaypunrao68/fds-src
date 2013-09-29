ROOTDIR = ../..
BOOSTIDIR = /usr/local/include/
BOOSTLDIR = /usr/local/lib/
IDIR = -I. -I$(ROOTDIR) -I$(ROOTDIR)/fds-include -I$(BOOSTIDIR)

CPPC = g++
OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
#OPTS ?= -O2 -DNDEBUG     # (C) Production mode

CFLAGS = $(IDIR) $(OPTS) -D_REENTRANT -std=c++0x

BOOST_LIBS = -L$(BOOSTLDIR) -lboost_thread -lboost_system
LIBS = $(BOOST_LIBS) -lpthread

SRCS = ThreadPool.cpp

OBJS = $(SRCS:%.cpp=%.o)
EXT_OBJS = $(ROOTDIR)/util/concurrency/Mutex.o

TESTS = threadpool

all: $(TESTS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

threadpool: ThreadPoolTest.o $(OBJS)
	$(CPPC) -o $@ ThreadPoolTest.o $(OBJS) $(EXT_OBJS) $(LIBS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TESTS)
