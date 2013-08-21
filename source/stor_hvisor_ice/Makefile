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

FDS_CFG		= StorHvisor.a

TARGETS		= $(FDS_CFG)

ICE_OBJS	= ../fdsp/FDSP.o

COBJS		= StorHvisorNet.o \
		  RPC_EndPoint.o \
		  StorHvisorTx.o  \
		  StorHvisorTbl.o \
		  StorHvisorRx.o
#		  VolumeCatalogCache.o \

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

#SLICE_SRCS	= FDSP.ice

#
# Use C++11 for additional STL support
#
CPP11		= yes

MY_FLAGS	= -std=c++0x -Wall -Werror -g2 -I../ -I../lib/Ice-3.5.0/cpp/include -I../fdsp/
MY_LIBS		= -L../lib/Ice-3.5.0/cpp/lib -lpthread -lIce -lIceUtil ../fdsp/FDSP.o ./VolumeCatalogCache.o ../fdsp/fds_pubsub.o -lIceStorm ../lib/OMgrClient.o -lboost_log -lboost_system -lboost_thread -lboost_date_time ../util/Log.o ../fdsp/fdsp_types.o ../util/hash/MurmurHash3.o

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. $(CPPFLAGS) -I $(top_srcdir)/include/Ice -I../include -I../util/hash -I../fds_client/include -I../fdsp -I.. -I../lib

# create only  .o for now 
$(FDS_CFG): $(OBJS) $(COBJS) $(ICE_OBJS)
	ar rc  $@  $(OBJS) $(COBJS) $(ICE_OBJS)
	$(CXX) $(LDFLAGS) -c -o VolumeCatalogCache.o VolumeCatalogCache.cpp $(MY_FLAGS)
	$(CXX) $(LDFLAGS) -o vcc_unit_test vcc_unit_test.cpp $(MY_FLAGS) $(MY_LIBS) $(FDS_CFG)
#$(CXX) $(LDFLAGS) -o sh_unit_test.cpp $@  $(OBJS) $(COBJS) $(ICE_OBJS) -L/usr/lib/mysql -L/usr/lib64/mysql -L/usr/lib -L../lib/  -L../lib/Ice-3.5.0/cpp/lib -lpthread -lIce -lIceUtil -lrt -lfds ../util/hash/MurmurHash3.o
#	rm -f $@
#	$(CXX)  $@ $(OBJS) $(COBJS) $(LIBS) $(HV_LIBS)
#	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(LIBS) $(HV_LIBS)
