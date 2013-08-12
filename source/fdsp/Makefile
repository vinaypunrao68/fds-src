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

TARGETS		= FDSP.o

OBJS		= FDSP.o 

SRCS		= $(OBJS:.o=.cpp) 

SLICE_SRCS	= FDSP.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)include/Ice \
	-I$(top_srcdir_ldb)/ \
	-I$(top_srcdir_ldb)include

.PHONY: clean

