topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cc          :=

user_cpp         := \
    fds_osdep.cpp \
    fds-shmem.cpp

user_ar_libs     := fds-osdep
fds-osdep        := $(user_cc) $(user_cpp)

include $(topdir)/Makefile.incl
