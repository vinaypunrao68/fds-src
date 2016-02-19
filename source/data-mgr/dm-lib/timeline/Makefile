topdir            := ../../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := $(topdir) ../../include
user_cpp_flags    :=
user_cpp          := $(wildcard *.cpp)

user_ar_libs      := fds-dm-lib
user_so_libs      := fds-dm-timeline
fds-dm-lib        := $(user_cpp)
fds-dm-timeline   := $(user_cpp)

include $(topdir)/Makefile.incl
