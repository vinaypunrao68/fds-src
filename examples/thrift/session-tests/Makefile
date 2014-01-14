la_SOURCES := \
	gen-cpp/DataPathReq.cpp \
	gen-cpp/DataPathReq.h \
	gen-cpp/DataPathResp.cpp \
	gen-cpp/DataPathResp.h \
	gen-cpp/fdsp_constants.cpp \
	gen-cpp/fdsp_constants.h \
	gen-cpp/fdsp_types.cpp \
	gen-cpp/fdsp_types.h

GEN_OBJ := $(patsubst %.cpp,%.o, $(la_SOURCES))

THRIFT_DIR := /usr/local/include/thrift
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -Igen-cpp

AM_CXXFLAGS = -Wall -std=c++0x

.PHONY: all clean

all: am_example sm_example

%.o: %.cpp
	$(CXX) $(AM_CXXFLAGS) -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

am_example: am_example.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

sm_example: sm_example.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

clean:
	$(RM) *.o am_example sm_example 
