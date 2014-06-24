probe_topdir      := ..
user_incl_dir     :=

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine
user_non_fds_libs :=

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    thrpool-api.cpp

user_bin_exe      := thpool-s3
thpool-s3         := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
