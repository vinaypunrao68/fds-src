thrift_dir        := ../../../../..
user_taget        := lib
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    :=
user_cc           :=
user_cpp          := \
    TServer.cpp \
    TSimpleServer.cpp \
    TThreadPoolServer.cpp \
    TThreadedServer.cpp \
    TNonblockingServer.cpp

user_ar_libs      := thrift
thrift            := $(user_cc) $(user_cpp)

include $(thrift_dir)/Makefile.incl
