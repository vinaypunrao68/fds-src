probe_topdir      := ..
user_incl_dir     :=

user_json         := o.json
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine
user_non_fds_libs :=

user_hh           := $(wildcard *.h)

# Don't do style check on auto gen code.
user_no_style     := $(patsubst %.json,%.h,$(user_json))
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    template_probe.cpp

user_bin_exe      := example-s3
example-s3        := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
