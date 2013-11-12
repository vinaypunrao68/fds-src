topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      :=
user_install_loc  := test

user_incl_dir     := ..
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-odb fds-thread fds-fdsp fds-hash
user_non_fds_libs :=                                                          \
    boost_thread boost_date_time boost_program_options leveldb                \
    Ice IceUtil IceStorm

user_cpp          := odb_unit_test.cpp
user_bin_exe      := odb_unit_test
odb_unit_test     := odb_unit_test.cpp

include $(topdir)/Makefile.incl
