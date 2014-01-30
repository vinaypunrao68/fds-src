topdir            := ../../..
probe_topdir      := ..
user_incl_dir     := $(topdir)/orch_mgr/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-om-common \
    fds-om-dlt

user_non_fds_libs :=

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    orch-mgr-probe.cpp

user_bin_exe      := orch-s3
orch-s3           := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
