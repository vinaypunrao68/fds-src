topdir           := ..
user_target      := lib
user_rtime_env   := user

user_thrift_loc  := $(topdir)/include/fdsp
user_thrift      := FDSP.thrift orch_proto.thrift
user_thrift_flag := --gen cpp:pure_enums --allow-64bit-consts

user_clean       := \
    $(wildcard $(user_thrift_loc)/*.h) \
    $(wildcard $(user_thrift_loc)/*.cpp) \
    $(user_thrift_loc)

include $(topdir)/Makefile.incl
