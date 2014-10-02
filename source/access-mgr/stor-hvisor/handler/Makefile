topdir            := ../../..
user_target       := lib
user_rtime_env    := user

# TODO: remove the topdir path in the include search
#
user_incl_dir     := \
    $(topdir) \
    $(topdir)/fds_client/include \
    $(topdir)/lib \
    $(topdir)/access-mgr/include

user_cpp          := $(wildcard *.cpp)
user_hh           := $(wildcard *.h)
user_no_style     := 

user_ar_libs      := fds-am-engine
fds-am-engine     := $(user_cpp)

include $(topdir)/Makefile.incl
