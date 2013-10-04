topdir            := ../..
user_target       := test
user_rtime_env    := user
user_build        :=

user_incl_dir     := ..
user_cpp_flags    := -DDMGR_LIVE_DEBUG
user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

user_cpp          :=                   \
    dm_unit_test.cpp

user_bin_exe      := dm_unit_test
dm_unit_test      := dm_unit_test.cpp

include $(topdir)/Makefile.incl
