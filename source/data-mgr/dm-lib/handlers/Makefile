topdir            := ../../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := 

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../../include
user_cpp_flags    := -DDMGR_LIVE_DEBUG
user_cpp          := $(wildcard *.cpp)
user_ar_libs      := fds-dm-lib
user_so_libs      := fds-dm-handlers
fds-dm-lib        := $(user_cpp)
fds-dm-handlers   := $(user_cpp)

include $(topdir)/Makefile.incl
