topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := ../stor_mgr.conf
user_install_loc  := test

user_incl_dir     := $(topdir) .. ../include 
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-lib fds-thread fds-util fds-fdsp fds-hash
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

user_cpp          := sm_unit_test.cpp rankeng_unit_test.cpp

user_bin_exe      := sm_unit_test rankeng_unit_test
sm_unit_test      := sm_unit_test.cpp
rankeng_unit_test := rankeng_unit_test.cpp ../ObjRank.cpp 

include $(topdir)/Makefile.incl
