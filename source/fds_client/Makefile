topdir            := ..
user_target       := lib
user_rtime_env    := user
user_install_loc  := exe

user_incl_dir     := include $(topdir)/stor-hvisor lib
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-StorHvisor \
    fds-lib \
    fds-umod \
    fds-thread \
    fds-util \
    fds-fdsp \
    fds-hash

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    thrift \
    config++

ubd_cc            :=                   \
    ubd.c

include $(topdir)/Makefile.incl

user_ar_libs      := fds-ubd
fds-ubd    := $(ubd_cc)

include $(topdir)/Makefile.incl
