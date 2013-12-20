topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := unit-test
user_install      := fds.conf om_client.conf
user_install_loc  := exe

user_incl_dir     := include $(topdir)/stor_hvisor_ice lib
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-StorHvisor fds-lib fds-umod fds-thread fds-util fds-fdsp fds-hash

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    boost_filesystem Ice IceUtil IceStorm

ubd_cc            :=                   \
    ubd.c

user_cc           := $(ubd_cc)
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := ubd
ubd               := $(ubd_cc)

include $(topdir)/Makefile.incl
