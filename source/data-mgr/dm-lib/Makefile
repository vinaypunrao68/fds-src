topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := refcount timeline dm-tvc dm-vol-cat handlers archive checker

user_fds_ar_libs := fds-lib

# boost_chrono and rabbitmq are used in the SimpleAmqpClient library
# The SimpleAmqpClient is used in the statsclient library
user_non_fds_libs := \
    fdsStatsUtil-debug \
    statsclient \
    SimpleAmqpClient \
    rabbitmq \
    ssl \
    boost_chrono \
    boost_system

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../include
user_cpp_flags    := -DDMGR_LIVE_DEBUG
user_cpp          := $(wildcard *.cpp)
user_ar_libs      := fds-dm-lib
user_so_libs      := fds-dm-lib
#.NOTPARALLEL: fds-dm-lib
fds-dm-lib        := $(user_cpp)

include $(topdir)/Makefile.incl
