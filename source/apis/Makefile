topdir           := ..
user_target      := lib
user_rtime_env   := user

user_thrift_loc  := $(topdir)/include/apis
user_thrift      := apis.thrift
user_thrift_flag := -r --gen cpp:pure_enums --allow-64bit-consts

user_thrift_py_loc  := $(topdir)/test/fdslib/pyfdsp
user_thrift_py_flag := -gen py

user_clean       := \
    $(wildcard $(user_thrift_loc)/*.h) \
    $(wildcard $(user_thrift_loc)/*.cpp) \
    $(user_thrift_loc)

include $(topdir)/Makefile.incl
