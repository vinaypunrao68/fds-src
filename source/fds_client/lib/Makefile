TOPDIR=../..
IDIR =-I$(TOPDIR)/include -I$(TOPDIR)/fds_client/include
CC=gcc
CFLAGS=$(IDIR) -g
LDIR =-L. -L$(TOPDIR)/lib
LIBS=$(LDIR) -lfds -lpthread

SRCS = hvisor_lib.c hvisor_lib_rx.c hvisor_tbl.c fbd_hash.c

OBJS = $(SRCS:%.c=%.o)

all: libhvisor.a

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

libhvisor.a: $(OBJS)
	ar -rcs $@ $(OBJS)

.PHONY: clean

clean:
	rm -f $(OBJS) libhvisor.a *~ core 
