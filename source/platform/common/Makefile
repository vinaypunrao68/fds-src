topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := ../include ../../net-service/include

user_cpp         := \
    platform.cpp \
    platform-proc.cpp \
    platform-shm-rw.cpp \
    plat-node-workflow.cpp

user_ar_libs     := fds-plat-daemon
fds-plat-daemon  := $(user_cpp)

include $(topdir)/Makefile.incl
