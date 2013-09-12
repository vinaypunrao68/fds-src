top_srcdir      = ../lib/Ice-3.5.0/cpp
CPP11	= -std=c++0x

OMGR       = cmdline_orch_mgr 

TARGETS         = $(OMGR)

OBJS            = ../fdsp/fdsp_types.o ../fdsp/fds_pubsub.o ../fdsp/FDSP.o

OMOBJS           = cmdline_orch_mgr.o

SRCS            = $(OMOBJS:.o=.cpp)

include $(top_srcdir)/config/Make.rules

CPPFLAGS        := -I. -I../fdsp $(CPP11) $(CPPFLAGS)
LIBS            := -lIceStorm $(LIBS)

$(OMGR): $(OBJS) $(OMOBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(OMOBJS) $(LIBS)

clean::
	rm -rf  cmdline_orch_mgr cmdline_orch_mgr.o
