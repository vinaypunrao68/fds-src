topdir            := ../..
user_target       := lib
user_rtime_env    := user

user_incl_dir     := ../include
user_cc_flags     :=
user_cc           :=                   \
    hvisor_lib.c                       \
    hvisor_lib_rx.c                    \
    hvisor_tbl.c                       \
    fbd_hash.c

user_ar_libs      := hvisor
hvisor            := $(user_cc)

include $(topdir)/Makefile.incl
