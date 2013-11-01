topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := orch-cli unit-test
user_install      := om_client.conf orch_mgr.conf
user_install_loc  := exe

user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-netproxy fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil leveldb

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir)

orch_mgr_cpp      :=                   \
    orchMgr.cpp                        \
    OmTier.cpp                         \
    OmVolPolicy.cpp		       \
    OmLocDomain.cpp		       \
    OmAdminCtrl.cpp

user_cpp          :=                   \
    $(orch_mgr_cpp)

user_bin_exe      := orchMgr
orchMgr           := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
