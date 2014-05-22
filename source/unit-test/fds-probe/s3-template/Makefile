probe_topdir      := ..
user_incl_dir     :=

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
		fds-StorHvisor
user_non_fds_libs :=

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    template_probe.cpp

user_bin_exe      := example-s3
example-s3        := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
