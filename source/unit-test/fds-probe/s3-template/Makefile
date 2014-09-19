probe_topdir      := ..
user_incl_dir     :=

user_json         := o.json
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := 
user_non_fds_libs :=

# Don't do style check on auto gen code.
user_hh           := $(wildcard *.h)
user_no_style     := $(patsubst %.json,%.h,$(user_json) adapter.h)

user_cc           := ngx_modules.c
user_cpp          := \
    test-main.cpp \
    adapter.cpp

user_bin_exe      := example-s3
example-s3        := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.svc
