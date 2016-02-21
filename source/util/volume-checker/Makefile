topdir            := ../../
user_target       := exe
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    := vclib

user_incl_dir     := include \
                     ../../testlib


user_fds_ar_libs  :=  \
    fds-svc-request \
    fds-umod \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-thread \
    fds-osdep \
    fds-test \
    fds-volume-checker

user_non_fds_libs :=  \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    boost_regex \
    thrift \
    jansson \
    config++ \
    crypto \
    hiredis \
    event \
    rt \
    fiu \
    dl \
    jemalloc

user_cc           :=
user_cpp          := $(wildcard *.cpp)

user_no_style     := $(user_cc)
user_bin_exe      := VolumeChecker
VolumeChecker     := $(user_cpp)

include $(topdir)/Makefile.incl
