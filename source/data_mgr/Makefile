topdir            := ..
user_target       := exe
user_rtime_env    := user
user_install      := dm_test.conf om_client.conf
user_install_loc  := exe
user_build_dir    := unit-test

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir)
user_cpp_flags    := -DDMGR_LIVE_DEBUG
user_fds_so_libs  :=
user_fds_ar_libs  :=fds-StorHvisor fds-lib fds-dsk-mgnt fds-umod fds-thread fds-util fds-fdsp
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_filesystem boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

data_mgr_cpp      :=                   \
    DataMgr.cpp                        \
    VolumeMeta.cpp

user_cpp          :=                   \
    $(data_mgr_cpp)

user_bin_exe      := DataMgr
DataMgr           := $(data_mgr_cpp)

include $(topdir)/Makefile.incl
