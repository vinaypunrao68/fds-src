topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := ../include /usr/include/fuse
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-probe-adapters fds-probe fds-dsk-mgnt fds-dsk-io fds-umod fds-thread  \
    fds-fdsp

user_non_fds_libs :=                                                          \
    boost_system boost_thread boost_program_options Ice IceUtil fuse

user_cc           := curse.c
user_bin_exe      := pm_blk_probe
pm_blk_probe      := curse.c

include $(topdir)/Makefile.incl
