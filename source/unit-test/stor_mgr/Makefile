topdir            := ../..
smdir             := $(topdir)/stor-mgr
probedir          := $(topdir)/fds-probe
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) $(topdir)/lib $(smdir)/include $(probedir)/include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                   \
    fds-smlib fds-am-engine fds-lib fds-odb fds-dsk-mgnt fds-umod fds-dsk-io \
    fds-thread fds-util fds-odb fds-vpolicy fds-netproxy fds-fdsp-ice

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_filesystem boost_thread boost_date_time      \
    boost_program_options Ice IceUtil IceStorm leveldb ssl crypto leveldb config++

stor_mgr_probe    := stormgr_probe_main.cpp
s3_sm_probe       := s3-sm-probe.cpp

user_cpp          :=                   \
    $(stor_mgr_probe)                  \
    $(s3_sm_probe)

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := StorMgrProbe s3-sm-probe
StorMgrProbe      := $(stor_mgr_probe)
s3-sm-probe       := $(s3_sm_probe)

include $(topdir)/Makefile.incl
