topdir            := ../../..
user_target       := test
user_rtime_env    := user
user_incl_dir     := $(topdir)/fds-probe/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-probe-adapters \
    fds-s3-probe \
    fds-probe \
    fds-am-engine \
    fds-StorHvisor \
    fds-lib \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-umod \
    fds-util \
    fds-hash \
    fds-fdsp-ice \
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
    s3-pm-probe.cpp

user_bin_exe      := s3-pm-probe
s3-pm-probe       := s3-pm-probe.cpp $(user_cc)

include $(topdir)/Makefile.incl
