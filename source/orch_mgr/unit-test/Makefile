topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := ..
user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb

user_cpp          :=                   \
    om_unit_test.cpp                   \
    place_tbl_unit_test.cpp

user_bin_exe      := om_test place_tbl_test
om_test           := om_unit_test.cpp
place_tbl_test    := place_tbl_unit_test.cpp

include $(topdir)/Makefile.incl
