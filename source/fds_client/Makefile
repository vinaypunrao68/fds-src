topdir            := ..
user_target       := lib
user_rtime_env    := user

user_incl_dir     := include $(topdir)/stor-hvisor lib
user_build_dir    := blk_dev
user_cpp_flags    :=
ubd_cc            :=                   \
    ubd.c

user_ar_libs      := fds-ubd
fds-ubd           := $(ubd_cc)

include $(topdir)/Makefile.incl
