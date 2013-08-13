# **********************************************************************
#
# Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************
top_srcdir	= ../lib/Ice-3.5.0/cpp
HV_LIBS		= ../util/hash/MurmurHash3.o

FDS_CFG		= StorHvisorNet

TARGETS		= $(FDS_CFG)

OBJS		= FDSP.o 

COBJS		= StorHvisorNet.o\
		  StorHvisorTx.o\
		  StorHvisorTbl.o\
		  StorHvisorRx.o

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

SLICE_SRCS	= FDSP.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)/include/Ice -I../include -I../util/hash -I../fds_client/include

# create only  .o for now 
$(FDS_CFG): $(OBJS) $(COBJS)
	rm -f $@
#	$(CXX)  $@ $(OBJS) $(COBJS) $(LIBS) $(HV_LIBS)
#	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(LIBS) $(HV_LIBS)
