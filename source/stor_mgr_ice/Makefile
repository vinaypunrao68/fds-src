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

OBJS		= StorMgr.o odb.o DiskMgr.o
ICE_OBJS	= ../fdsp/FDSP.o

MY_LIBS          = ../lib/leveldb-1.12.0/libleveldb.so -lboost_log -lboost_system -lboost_thread -lboost_date_time ../util/Log.o -lpthread -lssl -lcrypto

COBJS		= #StorHvisorClient.o

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

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
