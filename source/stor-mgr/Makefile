topdir            := ..
user_target       := exe
user_std_libs     := true
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    := smlib include utils

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := \
    $(topdir) \
    include \
    migrations \
    $(topdir)/lib \
    $(topdir)/persistent_layer/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                   \
    fds-smlib \
    fds-svc-request \
    fds-net-svc \
    fds-qos-lib \
    fds-odb \
    fds-dsk-mgnt \
    fds-smlib \
    fds-dsk-io \
    fds-dsk-mgnt \
    fds-odb \
    fds-hash \
    fds-net-svc \
    fds-fdsp

user_non_fds_libs := \
    boost_timer \
    leveldb \
    ssl \
    crypto \
    jemalloc \
    leveldb \
    sqlite3

stor_mgr_cpp      := stormgr_main.cpp
user_cpp          :=                   \
    $(stor_mgr_cpp)

user_bin_exe      := StorMgr
StorMgr           := $(stor_mgr_cpp)

include $(topdir)/Makefile.incl
