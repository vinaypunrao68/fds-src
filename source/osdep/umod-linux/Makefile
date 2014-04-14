topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    fds_osdep.c

user_cpp         :=
	fds-shmem.cpp \
	fds-shmtx.cpp

user_ar_libs     := fds-osdep
fds-osdep        := $(user_cc)

include $(topdir)/Makefile.incl
