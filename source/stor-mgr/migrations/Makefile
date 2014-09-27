topdir            := ../..
speeddup          := -j 3
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) ./ ../include $(topdir)/lib
user_cpp_flags    := -Wall
user_cpp          :=                   \
    TokenReceiver.cpp \
    TokenSender.cpp \
    fds_migration.cpp \
    TokenSyncReceiver.cpp \
    TokenSyncSender.cpp \
    TokenPullReceiver.cpp \
    TokenPullSender.cpp

user_ar_libs      := fds-migrations
fds-migrations    := $(user_cpp)

include $(topdir)/Makefile.incl
