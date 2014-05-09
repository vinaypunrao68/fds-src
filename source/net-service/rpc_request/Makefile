topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := $(topdir) . ../include
user_cpp         := \
	RpcRequest.cpp \
	AsyncRpcRequestTracker.cpp \
	RpcRequestPool.cpp \
	BaseAsyncSvcHandler.cpp

user_ar_libs     := fds-rpc-request
fds-rpc-request  := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
