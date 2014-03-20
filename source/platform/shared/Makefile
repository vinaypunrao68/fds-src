topdir           := ../..
user_target      := lib
user_rtime_env   := user
# user_cpp_flags   := -Wall -Werror
user_cc          :=
user_cpp         := \
    platform-lib.cpp \
    platform-rpc.cpp \
    platform-libmod.cpp \
    node-inventory.cpp

user_ar_libs     := fds-plat-lib
fds-plat-lib     := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
