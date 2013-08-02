XEN_ROOT=$(CURDIR)/../xen-4.2.2
BLKTAP_ROOT= $(XEN_ROOT)/tools/blktap2
CC = gcc

CFLAGS = -Iinclude -g

all: ubd 

ubd: ubd.o
	$(CC) -o ubd ubd.o

ubd.o: ubd.c
	$(CC) $(CFLAGS) -c -o ubd.o ubd.c

clean:
	rm *.o ubd

