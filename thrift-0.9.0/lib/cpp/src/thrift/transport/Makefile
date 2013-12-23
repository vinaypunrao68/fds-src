thrift_dir        := ../../../../..
user_taget        := lib
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    :=
user_cc           :=
user_cpp          := \
    TBufferTransports.cpp \
    TPipe.cpp \
    TSSLServerSocket.cpp \
    TFDTransport.cpp \
    TPipeServer.cpp \
    TSSLSocket.cpp \
    TFileTransport.cpp \
    TServerSocket.cpp \
    TTransportException.cpp \
    TSimpleFileTransport.cpp \
    TTransportUtils.cpp \
    TSocket.cpp \
    TZlibTransport.cpp \
    TSocketPool.cpp

user_ar_libs      := thrift
thrift            := $(user_cc) $(user_cpp)

include $(thrift_dir)/Makefile.incl
