topdir            := ..
user_target       := exe
user_rtime_env    := user
speedup           := -j 6
user_build_dir    := include platform
user_incl_dir     := $(topdir)/fds_client/include include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-am-engine \
    fds-probe-adapters \
    fds-s3-probe \
    fds-probe \
    fds-am-engine \
    fds-am-plat \
    fds-plat-lib \
    fds-net-svc \
    fds-svc-request \
    fds-umod \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-xdi \
    fds-hash \
    fds-thread \
    fds-osdep

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
    boost_regex \
    thrift \
    jansson \
    config++ \
    cryptopp \
    hiredis \
    event \
	rt \
    $(user_nginx_libs)

user_cc           :=  \
    ngx_modules.c

user_incl_dir     += $(topdir) \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_cpp      := $(wildcard *.cpp)

user_ld_flags := -shared -Wl,--no-allow-shlib-undefined

user_no_style     := $(user_cc) $(wildcard com_*.h)
user_bin_exe      := libaccessmgr.so
libaccessmgr.so   := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
