GEN_SRC := gen-cpp/sh_service.cpp gen-cpp/sh_constants.cpp gen-cpp/sh_types.cpp gen-cpp/sm_service.cpp gen-cpp/sm_constants.cpp gen-cpp/sm_types.cpp 
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

THRIFT_DIR := /usr/local/include/thrift
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR)

.PHONY: all clean

all: sm_server sh_server

%.o: %.cpp
	$(CXX) -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

sm_server: sm_server.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

sh_server: sh_server.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

clean:
	$(RM) *.o sm_server sh_server 
