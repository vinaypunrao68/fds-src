topdir            := ../..
probe_dir         := $(topdir)/fds-probe
user_target       := test
user_rtime_env    := user

user_incl_dir     := $(probe_dir)/include /usr/include/fuse
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-probe-adapters fds-probe-blk fds-probe fds-dsk-mgnt fds-dsk-io        \
    fds-umod fds-thread fds-fdsp

user_non_fds_libs :=                                                          \
    boost_system boost_thread boost_program_options Ice IceUtil fuse

user_cpp          :=                                                          \
    pm_probe_test.cpp sm_probe_test.cpp

user_bin_exe      := pm_blk_probe sm_blk_probe
pm_blk_probe      := pm_probe_test.cpp
sm_blk_probe      := sm_probe_test.cpp

include $(topdir)/Makefile.incl
