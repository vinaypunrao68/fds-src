topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) ./
user_cpp_flags    :=
user_cpp          :=                   \
    TokenCopySender.cpp \
    TokenCopyReceiver.cpp

user_no_style     :=
user_ar_libs      := fds-migrations
fds-migrations    := $(user_cpp)

include $(topdir)/Makefile.incl
