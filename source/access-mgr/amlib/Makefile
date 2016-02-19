topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../include
user_cpp_flags    :=
user_cpp          := $(wildcard *.cpp)

user_static_libs  := \
    ev

user_ar_libs      := fds-am-engine
fds-am-engine     := $(user_cpp)

include $(topdir)/Makefile.incl
