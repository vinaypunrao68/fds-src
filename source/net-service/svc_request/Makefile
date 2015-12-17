topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := $(topdir) . ../include
user_cpp         := \
	SvcMgr.cpp \
	SvcServer.cpp \
	SvcRequestTracker.cpp \
	SvcRequestPool.cpp \
	SvcRequest.cpp \
	SvcProcess.cpp \
	SvcPlatNetHandler.cpp

user_ar_libs     := fds-svc-request
user_so_libs     := fds-svc-request
fds-svc-request  := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
