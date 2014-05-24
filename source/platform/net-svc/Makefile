topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../include ../../net-service/include

net_platform_lib := \
    ep-map.cpp \
    net-service.cpp \
    net-platform.cpp \
    net-registration.cpp

user_cpp         := \
	$(net_platform_lib)

user_ar_libs     := fds-plat-daemon
fds-plat-daemon  := $(net_platform_lib)

include $(topdir)/Makefile.incl
