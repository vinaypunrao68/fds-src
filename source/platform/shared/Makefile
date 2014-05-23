topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := $(topdir)/net-service/include
user_cc          :=
user_cpp         := \
    platform-lib.cpp \
    platform-rpc.cpp \
    platform-libmod.cpp \
    node-inventory.cpp \
    node-inv-shmem.cpp \
    disk-inventory.cpp

user_ar_libs     := fds-plat-lib
fds-plat-lib     := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
