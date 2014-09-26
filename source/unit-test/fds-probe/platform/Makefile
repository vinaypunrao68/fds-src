probe_topdir      := ..
source_dir        := $(probe_topdir)/../..
user_incl_dir     := \
    $(source_dir)/platform/include \
    $(source_dir)/net-service/include

user_json         := node-work.json
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-plat-daemon \
    fds-plat-disk

user_non_fds_libs := \
    udev \
    rt \
    parted

user_hh           := $(wildcard *.h)
user_no_style     := $(patsubst %.json,%.h,$(user_json))
user_cc           := ngx_modules.c
user_cpp          := \
    plat-probe-main.cpp \
    plat-probe.cpp \
    adapter.cpp

user_bin_exe      := plat-probe
plat-probe        := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
