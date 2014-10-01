topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := $(topdir)/am-engine/include ../include $(topdir) ../stor-hvisor
user_cpp_flags    :=
user_cpp          := \
        AMSvcHandler.cpp\
	am-platform.cpp

user_ar_libs      := fds-am-plat
fds-am-plat       := $(user_cpp)

include $(topdir)/Makefile.incl
