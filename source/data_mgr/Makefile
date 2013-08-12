CC=g++
TOPDIR=..
TOPICEDIR = ../lib/Ice-3.5.0/cpp
IDIR =-I$(TOPDIR)/include -I$(TOPDIR) -I/usr/include/mysql/ -I$(TOPICEDIR)/include/Ice -I. -I$(TOPICEDIR)/include/ -I$(TOPDIR)/fdsp
LDIR =-L$(TOPDIR)/lib -L. -L/usr/lib/mysql -L/usr/lib64/mysql
CFLAGS=$(IDIR) -g -DDMGR_LIVE_DEBUG

OBJS		= dm.o data_mgr_transactions.o data_mgr_pvt.o DataMgr.o

#SLICE_SRCS	= FDSP.ice

DEPS = $(TOPDIR)/include/data_mgr.h $(TOPDIR)/include/fds_commons.h data_mgr_pvt.h

OTHER_OBJS = ../lib/vvclib.o ../lib/tvclib.o ../lib/vvc_db.o ../lib/vvc_private.o ../fdsp/FDSP.o

MY_LIBS = -lpthread -lssl -lcrypto -lmysqlclient 

top_srcdir	= ../lib/Ice-3.5.0/cpp
top_srcdir_ldb	= ../

FDS_CFG		= DataMgr
TARGETS		= $(FDS_CFG)

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

include $(TOPICEDIR)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I. -I $(top_srcdir)/include/Ice \
	-I$(top_srcdir_ldb)/ \
	-I/usr/include/mysql \
	-I./ \
	-I$(top_srcdir_ldb)/include \
	-I$(TOPDIR)/fdsp

$(FDS_CFG): $(OBJS) $(COBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(LIBS) $(LDIR) $(MY_LIBS) $(OTHER_OBJS)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

dmgrd: data_mgr.cpp data_mgr_transactions.o data_mgr_pvt.o
	$(CC) -o $@ $< $(LIBS) data_mgr_transactions.o data_mgr_pvt.o ../lib/vvclib.o ../lib/tvclib.o ../lib/vvc_db.o ../lib/vvc_private.o ../fdsp/FDSP.o $(CFLAGS)

unit_test: unit_test.o
	$(CC) -o $@ -g $<

