topdir            := ../../..
thriftdir         := ../../../..
user_target       := test
user_rtime_env    := user
user_incl_dir     := $(topdir)/fds-probe/include $(topdir)/am-engine $(thriftdir)/examples/thrift/perf-tests/gen-cpp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-probe-adapters \
    fds-probe \
    fds-am-engine \
    fds-StorHvisor \
    fds-lib \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-umod \
    fds-util \
    fds-hash \
    fds-fdsp \
    fds-thread

user_nginx_libs   :=  \
    ngx-http-module \
    ngx-http \
    ngx-event-module \
    ngx-event \
    ngx-core \
    ngx-os \
    ngx-core \
    pthread crypt pcre crypto z

user_non_fds_libs :=  \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    Ice \
    IceUtil \
    IceStorm \
    $(user_nginx_libs)

user_cc           := \
    ngx_modules.c

user_cpp          := \
    s3-pm-probe.cpp \
    s3-probe.cpp

user_no_style     := $(user_cpp)
user_bin_exe      := thrift-probe
thrift-probe      := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
