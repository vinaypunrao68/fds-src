topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := .. $(topdir)
user_cpp         := \
    copy_env.cpp

user_no_style    := \
    copy_env.cpp        \

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
