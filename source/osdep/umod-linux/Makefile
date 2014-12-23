topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    fds_osdep.c

user_cpp         := \
    fds-shmem.cpp

user_ar_libs     := fds-osdep
fds-osdep        := $(user_cc) $(user_cpp)

include $(topdir)/Makefile.incl
