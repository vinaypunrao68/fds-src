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

TARGETS		= fdsp_types.o FDSP.o fds_pubsub.o

OBJS		= fdsp_types.o FDSP.o fds_pubsub.o

SRCS		= $(OBJS:.o=.cpp) 

SLICE_SRCS	= fdsp_types.ice FDSP.ice fds_pubsub.ice

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)include/Ice \
	-I$(top_srcdir_ldb)/ \
	-I$(top_srcdir_ldb)include
SLICE2CPPFLAGS	:= --underscore -I. $(SLICE2CPPFLAGS)

.PHONY: clean

