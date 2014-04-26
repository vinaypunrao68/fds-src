topdir           := ..
user_target      := lib
user_rtime_env   := user

user_thrift_loc  := $(topdir)/include/xdi
user_thrift      := am_shim.thrift
user_thrift_flag := -r --gen cpp:pure_enums --allow-64bit-consts

user_clean       := \
    $(wildcard $(user_thrift_loc)/*.h) \
    $(wildcard $(user_thrift_loc)/*.cpp) \
    $(user_thrift_loc)

include $(topdir)/Makefile.incl
