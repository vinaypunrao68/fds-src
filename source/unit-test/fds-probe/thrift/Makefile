probe_topdir      := ..
loc_thrift_gen    := $(probe_topdir)/../../include/fdsp
loc_thrift_incl   := $(probe_topdir)/../../fdsp
user_thrift_loc   := thrift-gen
user_build_dir    := $(user_thrift_loc) server
user_incl_dir     := \
	$(user_thrift_loc) \
	$(loc_thrift_gen) \
    $(probe_topdir)/../../net-service/include \
	$(probe_topdir)/../../access-mgr/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=       \
    fds-svc-request \
    fds-net-svc \
    fds-am-engine  \
    fds-probe-thrift-utest

user_non_fds_libs :=

user_thrift       := mesg.thrift
user_thrift_flag  := --gen cpp:pure_enums --allow-64bit-consts -I $(loc_thrift_incl)

user_hh           := $(wildcard *.h)
user_cc           := \
    ngx_modules.c

local_probe_clnt  := \
    endpoint-test.cpp \
    am-endpoint.cpp \
    s3-thrift-probe.cpp \
    s3-thrift.cpp \
    $(user_cc)

user_cpp          := $(local_probe_clnt)
user_bin_exe      := thrift-probe
thrift-probe      := $(local_probe_clnt)

include $(probe_topdir)/Makefile.s3
