CC = gcc
TOPDIR = ..
CFLAGS = -Iinclude -g -I../stor_hvisor_ice
#LDIR = -L. -L$(TOPDIR)/fds_client/lib -L$(TOPDIR)/lib -L/usr/lib/mysql -L/usr/lib64/mysql -L/usr/lib  -L../lib/Ice-3.5.0/cpp/lib -L../stor_hvisor_ice
LDIR = -L. -L$(TOPDIR)/lib -L/usr/lib/mysql -L/usr/lib64/mysql -L/usr/lib  -L../lib/Ice-3.5.0/cpp/lib -L../stor_hvisor_ice
LIBS=$(LDIR) -lfds -lssl -lcrypto -lmysqlclient -lpthread  -lstdc++ -lIce -lIceUtil -lrt

all: ubd 

ubd: ubd.o
	$(CC) -o ubd ubd.o ../fdsp/FDSP.o ../util/hash/MurmurHash3.o ../stor_hvisor_ice/StorHvisorRx.o ../stor_hvisor_ice/StorHvisorTbl.o ../stor_hvisor_ice/StorHvisorTx.o ../stor_hvisor_ice/StorHvisorNet.o $(LIBS)

ubd.o: ubd.c
	$(CC) $(CFLAGS) -c -o ubd.o ubd.c

clean:
	rm *.o ubd

