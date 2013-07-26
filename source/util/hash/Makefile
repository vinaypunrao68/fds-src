TOPDIR=..
IDIR =-I.

CPPC =g++
#OPTS ?= -g2               # (A) Debug mode
#OPTS ?= -O2 -g2 -DNDEBUG # (B) Profiling mode
OPTS ?= -O2 -DNDEBUG     # (C) Production mode
CFLAGS = $(OPTS)
LIBS =

DEPS = 

SRCS = \
	AvalancheTest.cpp \
	Bitslice.cpp \
	Bitvec.cpp \
	City.cpp \
	CityTest.cpp \
	crc.cpp \
	DifferentialTest.cpp \
	Hashes.cpp \
	KeysetTest.cpp \
	lookup3.cpp \
	md5.cpp \
	MurmurHash1.cpp \
	MurmurHash2.cpp \
	MurmurHash3.cpp \
	Platform.cpp \
	Random.cpp \
	sha1.cpp \
	SpeedTest.cpp \
	Spooky.cpp \
	SpookyTest.cpp \
	Stats.cpp \
	SuperFastHash.cpp \
	Types.cpp

C_SRCS = \
	PMurHash.c

OBJS = $(SRCS:%.cpp=%.o)
C_OBJS = $(C_SRCS:%.c=%.o)

BINS = main

all: $(OBJS) $(C_OBJS) $(BINS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS)

main: main.cpp $(OBJS) $(C_OBJS)
	$(CPPC) -o $@ main.cpp $(OBJS) $(C_OBJS) $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(C_OBJS) $(BINS) *.core
