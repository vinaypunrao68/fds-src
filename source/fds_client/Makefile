CC = gcc
TOPDIR = ..
CFLAGS = -Iinclude -g -I../stor_hvisor_ice  -I$(TOPDIR)/include
#LDIR = -L. -L$(TOPDIR)/fds_client/lib -L$(TOPDIR)/lib -L/usr/lib/mysql -L/usr/lib64/mysql -L/usr/lib  -L../lib/Ice-3.5.0/cpp/lib -L../stor_hvisor_ice
LDIR = -L. -L$(TOPDIR)/lib -L/usr/lib/mysql -L/usr/lib64/mysql -L/usr/lib  -L../lib/Ice-3.5.0/cpp/lib -L../stor_hvisor_ice
LIBS=$(LDIR) -lfds -lssl -lcrypto -lmysqlclient -lpthread  -lstdc++ -lIce -lIceUtil -lrt -lm -lIceStorm -lboost_log -lboost_system -lboost_thread -lboost_date_time
UTEST_OBJS = ../fdsp/fdsp_types.o ../fdsp/fds_pubsub.o

all: ubd hvisor_uspace_test

ubd: ubd.o
	$(CC) -o ubd ubd.o ../fdsp/FDSP.o ../util/hash/MurmurHash3.o ../stor_hvisor_ice/RPC_EndPoint.o ../stor_hvisor_ice/StorHvisorRx.o ../stor_hvisor_ice/StorHvisorTx.o ../stor_hvisor_ice/StorHvisorTbl.o ../stor_hvisor_ice/StorHvisorNet.o ../stor_hvisor_ice/VolumeCatalogCache.o ../stor_hvisor_ice/StorHvJournal.o ../util/Log.o ../util/concurrency/Mutex.o $(UTEST_OBJS)  $(LIBS)

ubd_utest: ubd_utest.o
	$(CC) -o ubd_utest ubd_utest.o 

hvisor_uspace_test: ubd_hvisor_test.o
	$(CC) -o hvisor_uspace_test ubd_hvisor_test.o ../fdsp/FDSP.o ../util/hash/MurmurHash3.o ../stor_hvisor_ice/RPC_EndPoint.o ../stor_hvisor_ice/StorHvisorRx.o ../stor_hvisor_ice/StorHvisorTx.o ../stor_hvisor_ice/StorHvisorTbl.o ../stor_hvisor_ice/StorHvisorNet.o ../stor_hvisor_ice/VolumeCatalogCache.o ../stor_hvisor_ice/StorHvJournal.o ../util/Log.o ../util/concurrency/Mutex.o $(UTEST_OBJS)  $(LIBS)

ubd_hvisor_test.o: ubd.c
	$(CC) -c -o ubd_hvisor_test.o ubd.c $(CFLAGS) -DHVISOR_USPACE_TEST

ubd_utest.o: ubd.c
	$(CC) -c -o ubd_utest.o ubd.c $(CFLAGS)  -DBLKTAP_UNIT_TEST -DBLKTAP_UNIT_TEST_CRASH 

ubd.o: ubd.c
	$(CC) $(CFLAGS) -c -o ubd.o ubd.c

clean:
	rm *.o ubd ubd_utest hvisor_uspace_test

