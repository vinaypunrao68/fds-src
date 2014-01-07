GEN_SRC := fds_constants.cpp FDSP_DataPathResp.cpp FDSP_DataPathReq.cpp fds_types.cpp
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

THRIFT_DIR := /usr/local/include/thrift
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR)

.PHONY: all clean

all: sh_server sm_server

%.o: %.cpp
	$(CXX) -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

sh_server: sh_server.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

sm_server: sm_server.o $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift 

clean:
	$(RM) *.o sh_server sm_server
