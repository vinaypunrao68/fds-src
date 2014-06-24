probe_topdir      := ..
user_thrift_loc   := thrift-gen
user_build_dir    := $(user_thrift_loc)
user_incl_dir     := $(user_thrift_loc)

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=       \
    fds-am-engine          \
    fds-probe-thrift-utest

user_non_fds_libs :=

user_thrift       := mesg.thrift
user_thrift_flag  := --gen cpp:pure_enums --gen java --allow-64bit-consts
user_hh           := \
    s3-thrift-probe.h

user_cc           := \
    ngx_modules.c

local_probe_clnt  := \
    s3-thrift-probe.cpp \
    s3-thrift.cpp \
    $(user_cc)

local_server      := \
    thrf-server.cpp

user_cpp          := \
    $(local_server) \
    $(local_probe_clnt)

user_bin_exe      := thrift-probe thrift-server
thrift-probe      := $(local_probe_clnt)
thrift-server     := $(local_server)

include $(probe_topdir)/Makefile.s3
