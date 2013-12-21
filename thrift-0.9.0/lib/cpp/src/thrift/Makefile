thrift_dir        := ../../../..
user_taget        := lib
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    := async concurrency processor protocol server transport
user_cc           :=
user_cpp          := \
    Thrift.cpp \
    TApplicationException.cpp \
    VirtualProfiling.cpp

user_ar_libs      := thrift
thrift            := $(user_cc) $(user_cpp)

include $(thrift_dir)/Makefile.incl
