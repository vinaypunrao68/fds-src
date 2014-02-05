topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := orch-cli include om-dlt om-common om-lib
user_install      := om_client.conf orch_mgr.conf
user_install_loc  := exe

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-om-common \
    fds-om-lib \
    fds-om-dlt \
    fds-thread \
    fds-lib \
    fds-util \
    fds-netproxy \
    fds-fdsp \
    fds-umod

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    config++ \
    thrift

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir) include

orch_mgr_cpp      :=                   \
    orch-mgr-main.cpp

user_hh           := $(wildcard *.h)
user_cpp          := $(orch_mgr_cpp)
user_bin_exe      := orchMgr
orchMgr           := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
