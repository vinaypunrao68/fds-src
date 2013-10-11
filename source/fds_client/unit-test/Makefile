topdir            := ../..
user_target       := test
user_rtime_env    := user
user_build_dir    :=
user_install      := ../om_client.conf ../fds.conf
user_install_loc  := test

user_incl_dir     := ../ ../include $(topdir)/stor_hvisor_ice
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-StorHvisor fds-lib fds-thread fds-util fds-fdsp fds-hash

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    boost_filesystem Ice IceUtil IceStorm

ubd_test_cc       :=                   \
    ubd_test.c

user_cc            := $(ubd_test_cc)
user_bin_exe       := hvisor_uspace_test
hvisor_uspace_test := $(ubd_test_cc)

include $(topdir)/Makefile.incl
