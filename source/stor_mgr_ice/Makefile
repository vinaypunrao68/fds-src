topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := unit-test
user_install_loc  := exe
user_install      := config.server om_client.conf stor_mgr.conf stor_mgr.cfg

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir)
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

stor_mgr_cpp      :=                   \
    StorMgr.cpp                        \
    odb.cpp                            \
    DiskMgr.cpp                        \
    StorMgrVolumes.cpp

user_cpp          :=                   \
    $(stor_mgr_cpp)

user_bin_exe      := StorMgr
StorMgr           := $(stor_mgr_cpp)

include $(topdir)/Makefile.incl
