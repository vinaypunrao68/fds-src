probe_topdir      := ../..
user_build_dir    :=
user_incl_dir     := \
    ../ \
    ../thrift-gen \
    $(probe_topdir)/../../include/fdsp \
    $(probe_topdir)/../../platform/include \
    $(probe_topdir)/../../net-service/include \
    $(probe_topdir)/../../access-mgr/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-svc-request \
    fds-net-svc \
    fds-probe-thrift-utest

user_non_fds_libs := \
    udev \
    rt \
    parted

user_hh           := $(wildcard *.h)
user_cc           := ../ngx_modules.c
local_server      := \
    ../endpoint-test.cpp \
    sm-endpoint.cpp \
    thrf-server.cpp \
    $(user_cc)

user_cpp          := $(local_server)
user_bin_exe      := thrift-server
thrift-server     := $(local_server)

include $(probe_topdir)/Makefile.s3
