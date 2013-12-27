thrift_dir        := ../../../../..
user_taget        := lib
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    :=
user_cc           :=
user_cpp          := \
    TDebugProtocol.cpp \
    TDenseProtocol.cpp \
    TJSONProtocol.cpp \
    TBase64Utils.cpp

user_ar_libs      := thrift
thrift            := $(user_cc) $(user_cpp)

include $(thrift_dir)/Makefile.incl
