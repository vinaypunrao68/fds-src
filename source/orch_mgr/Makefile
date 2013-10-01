CC=g++
TOPDIR=..
TOPICEDIR = ../lib/Ice-3.5.0/cpp
IDIR =-I$(TOPDIR)/include -I$(TOPDIR) -I$(TOPICEDIR)/include/Ice -I. -I$(TOPICEDIR)/include/ -I$(TOPDIR)/fdsp -I$(TOPDIR)/lib/leveldb-1.12.0/
LDIR =-L$(TOPDIR)/lib -L. -L$(TOPDIR)/util/ -L$(TOPDIR)/lib/leveldb-1.12.0/
CFLAGS=$(IDIR) -g 

OBJS		= orchMgr.o


DEPS = $(TOPDIR)/util/Log.h

UTIL_DIR   = ../util/
UTIL_OBJS  = $(UTIL_DIR)/Log.o
BOOST_LIBS = -lboost_log -lboost_system -lboost_thread -lboost_date_time -lboost_program_options
SYS_LIBS   = -lpthread -lssl -lcrypto
LEVELDB_LIBS = -lleveldb

LIB_DIR    = ../lib/
FDS_OBJS   = $(UTIL_OBJS) ../fdsp/FDSP.o ../util/concurrency/Mutex.o
OM_OBJS    = ../fdsp/FDSP.o

MY_LIBS = $(BOOST_LIBS) $(SYS_LIBS) $(LEVELDB_LIBS)

top_srcdir	= ../lib/Ice-3.5.0/cpp
top_srcdir_ldb	= ../

FDS_CLI		= fdscli
FDS_CFG		= orchMgr
#OMGR		= cmdline_orch_mgr
OM_UTEST	= om_unit_test
PT_UTEST        = place_tbl_unit_test
OMOBJS          = cmdline_orch_mgr.o
CLI_OBJ		= fdscli.o
TARGETS		= $(FDS_CFG) $(OMGR) $(OM_UTEST) $(FDS_CLI) $(PT_UTEST)

SRCS		= $(OBJS:.o=.cpp) \
		  $(COBJS:.o=.cpp)

#
# Use C++11 for additional STL support
#
CPP11	= -std=c++0x

TEST_FLAGS	= -std=c++0x -Wall -Werror -g2 -I../ -I../lib/Ice-3.5.0/cpp/include -I../fdsp/
TEST_LIBS	= -L../lib/Ice-3.5.0/cpp/lib -lpthread -lIce -lIceUtil ../fdsp/FDSP.o -lboost_log -lboost_system -lboost_thread -lboost_date_time ../util/Log.o

include $(TOPICEDIR)/config/Make.rules
LIBS            := -lIceStorm $(LIBS)

CPPFLAGS	:= -I. $(CPPFLAGS) -I. -I $(top_srcdir)/include/Ice \
	-I$(top_srcdir_ldb)/ \
	-I./ \
	-I$(top_srcdir_ldb)/include \
	-I$(TOPDIR)/fdsp \
	-I$(top_srcdir_ldb)/util \
	-I/usr/local/include \
	-I$(TOPDIR)/lib/leveldb-1.12.0/ \
	-I$(TOPDIR)/lib/leveldb-1.12.0/include/ \
	$(CPP11)

#$(OMGR): $(OM_OBJS) $(OMOBJS)
#	$(CXX) $(LDFLAGS) -o $@ $(OM_OBJS) $(OMOBJS) $(LIBS) $(MYLIBS)

$(FDS_CLI): $(OM_OBJS) $(CLI_OBJ)
	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(OM_OBJS) $(CLI_OBJ) $(LIBS) $(MYLIBS) $(MY_LIBS) $(UTIL_OBJS) $(LDIR)

$(OM_UTEST): om_unit_test.o
	$(CXX) $(LDFLAGS) -o $@ om_unit_test.o $(FDS_OBJS) $(LIBS) $(MY_LIBS) $(LDIR)

$(PT_UTEST): place_tbl_unit_test.cpp
	$(CXX) $(LDFLAGS) -o $@ place_tbl_unit_test.cpp $(CPP11) $(FDS_OBJS) $(LIBS) $(MY_LIBS) $(IDIR) $(LDIR)

$(FDS_CFG): $(OBJS) $(COBJS)
#	rm -f $@
	$(CXX) $(LDFLAGS) -o $@ $(MY_LIBS) $(OBJS) $(COBJS) $(LIBS) $(LDIR) $(FDS_OBJS) -I../ -I/usr/local/include -I$(top_srcdir)/include/Ice $(IDIR)
#	$(CXX) $(LDFLAGS) -o dm_unit_test dm_unit_test.cpp $(TEST_FLAGS) $(TEST_LIBS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) -c -o $@ $< $(CFLAGS) $(LIBS)

unit_test: unit_test.o
	$(CC) -o $@ -g $<

