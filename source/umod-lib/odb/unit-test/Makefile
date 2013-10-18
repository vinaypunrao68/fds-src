topdir            := ../../..
user_target       := test
user_rtime_env    := user
user_install      :=
user_install_loc  := test

user_incl_dir     := ..
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-lib fds-umod fds-thread fds-odb fds-fdsp fds-hash
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

user_cpp          := odb_unit_test.cpp
user_bin_exe      := odb_unit_test
odb_unit_test     := odb_unit_test.cpp

include $(topdir)/Makefile.incl
