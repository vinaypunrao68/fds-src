# **********************************************************************
#
# Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************
top_srcdir	= ..
top_srcdir	= ../lib/Ice-3.5.0/cpp

FDS_CFG		= StorHvisorClient

TARGETS		= $(FDS_CFG)

OBJS		= FDSP.o

COBJS		= StorHvisorClient.o

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

FDS_SLICE_FLAG	= --underscore
SLICE_SRCS	= FDSP.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)/include/Ice

$(FDS_CFG): $(OBJS) $(COBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(LIBS)
