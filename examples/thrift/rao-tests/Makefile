GEN_SRC := gen-cpp/sh_service.cpp gen-cpp/sh_constants.cpp gen-cpp/sh_types.cpp gen-cpp/sm_service.cpp gen-cpp/sm_constants.cpp gen-cpp/sm_types.cpp 
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

THRIFT_DIR := ../../../thrift-0.9.0/lib/cpp/src
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR)

.PHONY: all clean

all: sm_server sh_server

%.o: %.cpp
	$(CXX) -std=c++11 -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

sm_server: sm_server.o $(GEN_OBJ)
	$(CXX) $^ -std=c++11 -o $@ -L/usr/local/lib -lthrift 

sh_server: sh_server.o $(GEN_OBJ)
	$(CXX) $^ -std=c++11 -o $@ -L/usr/local/lib -lthrift 

clean:
	$(RM) *.o sm_server sh_server 
