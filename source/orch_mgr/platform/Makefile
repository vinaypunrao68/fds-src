topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := \
	../include \
    $(topdir)/net-service/include

user_cpp_flags    :=
user_cpp          := \
	om-platform.cpp \
    om-svc-handler.cpp

user_ar_libs      := fds-om-plat
fds-om-plat       := $(user_cpp)

include $(topdir)/Makefile.incl
