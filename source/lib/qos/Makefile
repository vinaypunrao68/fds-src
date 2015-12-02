topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         :=      \
    qos_ctrl.cpp

user_cpp_flags   := -Wall -Werror -Wno-reorder
user_incl_dir    := $(topdir) $(topdir)/lib
user_so_libs     := fds-qos-lib
fds-qos-lib          := $(user_cpp)


include $(topdir)/Makefile.incl
