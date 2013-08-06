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

OBJS		= FDSP.o StorMgr.o odb.o DiskMgr.o

MY_LIBS          = /home/sanjay/fds11/source/lib/leveldb-1.12.0/libleveldb.so

COBJS		= #StorHvisorClient.o

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

SLICE_SRCS	= FDSP.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)include/Ice \
	-I$(top_srcdir_ldb)lib/leveldb-1.12.0/include \
	-I$(top_srcdir_ldb)lib/leveldb-1.12.0 \
	-I$(top_srcdir_ldb)/ \
	-I$(top_srcdir_ldb)include

$(FDS_CFG): $(OBJS) $(COBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(LIBS) $(MY_LIBS)
