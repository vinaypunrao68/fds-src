topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := $(topdir)/stor_mgr_ice/stor_mgr.conf
user_install_loc  := test

user_incl_dir     := $(topdir) $(topdir)/stor_mgr_ice
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-lib \
    fds-thread \
    fds-util \
    fds-fdsp-ice \
    fds-hash \
    fds-probe-adapters \
    fds-probe-blk \
    fds-probe \
    fds-umod

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb fuse

user_cpp          := sm_unit_test.cpp 
user_no_style     := $(user_cpp) $(user_cc)

user_bin_exe      := sm_unit_test 
sm_unit_test      := sm_unit_test.cpp

include $(topdir)/Makefile.incl
