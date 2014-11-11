probe_topdir      := ..
local_topdir      := $(probe_topdir)/../..

user_install_loc  := test
user_install      := sm.conf

user_incl_dir     := \
    $(local_topdir)/stor-mgr/include \
    $(local_topdir) \
    $(local_topdir)/stor-mgr/migrations \
    $(local_topdir)/lib \
		/usr/include/cryptopp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib \
    fds-migrations \
    fds-am-engine \
    fds-sm-plat \
    fds-smlib \
    fds-plat-lib \
    fds-lib \
    fds-vpolicy \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-odb

user_non_fds_libs := \
    leveldb \
    hiredis

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    sm-probe.cpp

user_bin_exe      := sm-probe-full
sm-probe-full     := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
