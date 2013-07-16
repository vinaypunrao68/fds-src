TOPDIR=..
IDIR =-I. -I$(TOPDIR)/include 
CC=gcc
CFLAGS=$(IDIR) -g
LIBS=-lpthread

DEPS = 

SRCS = stor_mgr.c disk_mgr.c

OBJS = $(SRCS:%.c=%.o)

ALL: stor_mgr 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

stor_mgr: stor_mgr.o disk_mgr.o 
	$(CC) -o $@ $(OBJS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(UTEST_OBJS) *~ core 
