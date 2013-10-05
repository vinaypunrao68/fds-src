topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

user_incl_dir     := include $(topdir)/stor_hvisor_ice
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-StorHvisor fds-thread fds-util fds-fdsp fds-lib fds-hash

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    boost_filesystem Ice IceUtil IceStorm

user_cpp          :=
ubd_cc            :=                   \
    ubd.c

user_bin_exe      := ubd
ubd               := $(ubd_cc)

include $(topdir)/Makefile.incl
