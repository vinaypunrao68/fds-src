probe_topdir      := ..
user_build_dir    :=
user_incl_dir     :=

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine

user_non_fds_libs :=

user_hh           := \
    fds-net-probe.h

user_cc           := \
    ngx_modules.c

local_probe_clnt  := \
    fds-net-probe.cpp \
    fds-net.cpp \
    $(user_cc)

local_server      := \
    fds-net-server.cpp

user_cpp          := \
    $(local_server) \
    $(local_probe_clnt)

user_no_style     := fds-net-server.cpp
user_bin_exe      := fds-net-probe fds-net-server
fds-net-probe     := $(local_probe_clnt)
fds-net-server    := $(local_server)

include $(probe_topdir)/Makefile.s3
