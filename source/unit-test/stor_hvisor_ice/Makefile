topdir            := ../..
hvisor_dir        := $(topdir)/stor_hvisor_ice
user_target       := test
user_rtime_env    := user

# TODO: remove the topdir from include search path.
#
user_incl_dir     :=                          \
    $(hvisor_dir)                             \
    $(topdir)/fds_client/include              \
    $(topdir)

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-StorHvisor fds-lib fds-thread fds-util fds-fdsp fds-hash

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    boost_filesystem Ice IceUtil IceStorm leveldb ssl crypto leveldb

user_cpp          :=                   \
    vcc_unit_test.cpp

user_bin_exe      := vcc_unit_test
vcc_unit_test     := vcc_unit_test.cpp

include $(topdir)/Makefile.incl
