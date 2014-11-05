probe_topdir      := ..
source_dir        := $(probe_topdir)/../..
user_incl_dir     := \
    $(source_dir)/access-mgr/stor-hvisor \
    $(source_dir)/access-mgr/include \
    $(source_dir)/net-service/include \
		/usr/include/cryptopp

user_json         := in.json
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := 
user_non_fds_libs :=
user_static_libs  := ev

# Don't do style check on auto gen code.
user_hh           := $(wildcard *.h)
user_no_style     := $(patsubst %.json,%.h,$(user_json) adapter.h)

user_cc           := ngx_modules.c
user_cpp          := \
    am-probe.cpp \
    probe-adapter.cpp \
    adapter.cpp

user_bin_exe      := am-probe
am-probe          := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.svc
