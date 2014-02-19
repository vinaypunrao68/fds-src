probe_topdir      := ..
local_topdir      := $(probe_topdir)/../..
user_incl_dir     := \
    $(local_topdir)/orch_mgr/include \
    $(local_topdir)

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-om-dlt \
    fds-om-lib \
    fds-om-common \
	fds-plat-lib

user_non_fds_libs := \
    leveldb

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    orch-mgr-probe.cpp

user_bin_exe      := orch-s3
orch-s3           := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
