topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../include ../../net-service/include
user_cpp         := \
    ep-map.cpp \
    node-inv-shmem.cpp \
    net-service.cpp \
    net-platform.cpp \
    net-registration.cpp

user_ar_libs     := fds-net-platd
fds-net-platd    := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
