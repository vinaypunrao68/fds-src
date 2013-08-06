CC = gcc
TOPDIR = ..
CFLAGS = -Iinclude -g
LDIR = -L. -L$(TOPDIR)/fds_client/lib -L$(TOPDIR)/lib -L/usr/lib/mysql -L/usr/lib64/mysql
LIBS=$(LDIR) -lhvisor -lfds -lssl -lcrypto -lmysqlclient -lpthread

all: ubd 

ubd: ubd.o
	$(CC) -o ubd ubd.o $(LIBS)

ubd.o: ubd.c
	$(CC) $(CFLAGS) -c -o ubd.o ubd.c

clean:
	rm *.o ubd

