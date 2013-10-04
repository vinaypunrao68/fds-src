topdir            := ../..
user_target       := test
user_rtime_env    := user

# TODO: remove the topdir from include search path.
#
user_incl_dir     := .. $(topdir)/fds_client/include $(topdir)
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

user_cpp          :=                   \
    vcc_unit_test.cpp

user_ar_libs      := vcc_unit_test
vcc_unit_test     := vcc_unit_test.cpp

include $(topdir)/Makefile.incl
