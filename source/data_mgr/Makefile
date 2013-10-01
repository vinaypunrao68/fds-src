CC=g++
TOPDIR=..
TOPICEDIR = ../lib/Ice-3.5.0/cpp
IDIR =-I$(TOPDIR)/include -I$(TOPDIR) -I$(TOPICEDIR)/include/Ice -I. -I$(TOPICEDIR)/include/ -I$(TOPDIR)/fdsp -I$(TOPDIR)/lib/leveldb-1.12.0/ -I$(TOPDIR)/lib/leveldb-1.12.0/include/
LDIR =-L$(TOPDIR)/lib -L. -L$(TOPDIR)/util/ -L$(TOPDIR)/lib/leveldb-1.12.0/
CFLAGS=$(IDIR) -g -DDMGR_LIVE_DEBUG

#OBJS		= dm.o data_mgr_transactions.o data_mgr_pvt.o DataMgr.o
OBJS		= DataMgr.o VolumeMeta.o

#SLICE_SRCS	= FDSP.ice

DEPS = $(TOPDIR)/include/data_mgr.h $(TOPDIR)/include/fds_commons.h data_mgr_pvt.h $(TOPDIR)/util/Log.h

UTIL_DIR   = ../util/
UTIL_OBJS  = $(UTIL_DIR)/Log.o
BOOST_LIBS = -lboost_log -lboost_system -lboost_thread -lboost_date_time
SYS_LIBS   = -lpthread -lssl -lcrypto
#MYSQL_LIBS = -lmysqlclient
LEVELDB_LIBS = -lleveldb

#LIB_OBJS   = $(LIB_DIR)/vvclib.o $(LIB_DIR)/tvclib.o $(LIB_DIR)/vvc_db.o $(LIB_DIR)/vvc_private.o
LIB_DIR    = ../lib/
LIB_OBJS   = $(LIB_DIR)/VolumeCatalog.o $(LIB_DIR)/Catalog.o
FDS_OBJS   = $(LIB_OBJS) $(UTIL_OBJS) ../fdsp/FDSP.o ../util/concurrency/ThreadPool.o ../util/concurrency/Mutex.o ../util/concurrency/Synchronization.o $(TOPDIR)/lib/OMgrClient.o

MY_LIBS = $(BOOST_LIBS) $(SYS_LIBS) $(LEVELDB_LIBS) -lIceStorm

top_srcdir	= ../lib/Ice-3.5.0/cpp
top_srcdir_ldb	= ../

FDS_CFG		= DataMgr
TARGETS		= $(FDS_CFG)

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

#
# Use C++11 for additional STL support
#
CPP11		= yes

TEST_FLAGS	= -std=c++0x -Wall -Werror -g2 -I../ -I../lib/Ice-3.5.0/cpp/include -I../fdsp/
TEST_LIBS	= -L../lib/Ice-3.5.0/cpp/lib -lpthread -lIce -lIceUtil ../fdsp/FDSP.o -lboost_log -lboost_system -lboost_thread -lboost_date_time ../util/Log.o

include $(TOPICEDIR)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I. -I $(top_srcdir)/include/Ice \
	-I$(top_srcdir_ldb)/ \
	-I/usr/include/mysql \
	-I./ \
	-I$(top_srcdir_ldb)/include \
	-I$(TOPDIR)/fdsp \
	-I$(top_srcdir_ldb)/util \
	-I/usr/local/include \
	-I$(TOPDIR)/lib/leveldb-1.12.0/ \
	-I$(TOPDIR)/lib/leveldb-1.12.0/include/ \
	-I$(TOPDIR)/fds-include/

$(FDS_CFG): $(OBJS) $(COBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(MY_LIBS) $(OBJS) $(COBJS) $(LIBS) $(LDIR) $(FDS_OBJS) -I../ -I/usr/local/include -I$(top_srcdir)/include/Ice $(IDIR)
	$(CXX) $(LDFLAGS) -o dm_unit_test dm_unit_test.cpp $(TEST_FLAGS) $(TEST_LIBS)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

dmgrd: data_mgr.cpp data_mgr_transactions.o data_mgr_pvt.o
	$(CC) -o $@ $< $(LIBS) data_mgr_transactions.o data_mgr_pvt.o ../lib/vvclib.o ../lib/tvclib.o ../lib/vvc_db.o ../lib/vvc_private.o ../fdsp/FDSP.o $(CFLAGS)

unit_test: unit_test.o
	$(CC) -o $@ -g $<

