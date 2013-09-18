# **********************************************************************
#
# Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************

top_srcdir	= ../lib/Ice-3.5.0/cpp
top_srcdir_ldb	= ../

FDS_CFG		= StorMgr

TARGETS		= $(FDS_CFG)

OBJS		= StorMgr.o odb.o DiskMgr.o  StorMgrVolumes.o
ICE_OBJS	= ../fdsp/FDSP.o ../fdsp/fds_pubsub.o ../fdsp/fdsp_types.o ../lib/OMgrClient.o

MY_LIBS          = ../lib/leveldb-1.12.0/libleveldb.so -lboost_log -lboost_system -lboost_thread -lboost_date_time \
                   ../util/Log.o -lIce -lIceUtil -lIceStorm -lpthread -lssl -lcrypto  ../util/concurrency/Mutex.o 

COBJS		= #StorHvisorClient.o

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

#
# Use C++11 for additional STL support
#
CPP11		= yes

TEST_FLAGS	= -std=c++0x -Wall -Werror -g2 -I../ -I../lib/Ice-3.5.0/cpp/include -I../fdsp/
TEST_LIBS	= -L../lib/Ice-3.5.0/cpp/lib -lpthread -lIceStorm -lIce -lIceUtil ../fdsp/FDSP.o -lboost_log -lboost_system -lboost_thread -lboost_date_time ../util/Log.o ../util/concurrency/Mutex.o

# SLICE_SRCS	= FDSP.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)include/Ice \
	-I$(top_srcdir_ldb)lib/leveldb-1.12.0/include \
	-I$(top_srcdir_ldb)lib/leveldb-1.12.0 \
	-I$(top_srcdir_ldb)\
	-I/usr/local/include \
	-I$(top_srcdir_ldb)include \
	-I../fdsp

$(FDS_CFG): $(OBJS) $(COBJS) $(ICE_OBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(ICE_OBJS) $(LIBS) $(MY_LIBS)
	$(CXX) $(LDFLAGS) -o sm_unit_test sm_unit_test.cpp $(TEST_FLAGS) $(TEST_LIBS)
