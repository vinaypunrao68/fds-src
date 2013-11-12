topdir            := ../..
smdir             := $(topdir)/stor_mgr_ice
probedir          := $(topdir)/fds-probe
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) $(smdir)/include $(probedir)/include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                   \
    fds-smlib fds-StorHvisor fds-lib fds-odb fds-dsk-mgnt fds-umod fds-dsk-io \
    fds-thread fds-util fds-odb fds-vpolicy fds-netproxy fds-fdsp

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_filesystem boost_thread boost_date_time      \
    boost_program_options Ice IceUtil IceStorm leveldb ssl crypto leveldb

stor_mgr_probe    := stormgr_probe_main.cpp

user_cpp          :=                   \
    $(stor_mgr_probe)

user_bin_exe      := StorMgrProbe
StorMgrProbe      := $(stor_mgr_probe)

include $(topdir)/Makefile.incl
