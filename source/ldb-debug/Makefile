topdir            := ..
user_target       := exe
user_std_libs     := true
user_rtime_env    := user

user_incl_dir     := include $(topdir)
user_cpp_flags    :=
user_fds_so_libs  :=

user_non_fds_libs := \
    leveldb

user_cpp          := ldb-debug.cpp
user_bin_exe      := ldb-debug
ldb-debug           := $(user_cpp)

include $(topdir)/Makefile.incl
