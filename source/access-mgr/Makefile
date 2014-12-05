topdir            := ..
user_target       := exe
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    := \
    include \
    amlib \
    stor-hvisor \
    platform \
    block \
    test

user_incl_dir     := \
	stor-hvisor \
	include \
	/usr/include/cryptopp

user_fds_ar_libs  :=  \
    fds-am-engine \
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
    $(user_nginx_libs) \
    fiu \
    dl \
    tcmalloc

user_static_libs  := \
    ev

user_cc           :=

user_incl_dir     += $(topdir) \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_cpp      := $(wildcard *.cpp)

#user_ld_flags := -shared -Wl,--no-allow-shlib-undefined

user_ld_flags := -lprofiler

user_no_style     := $(user_cc) $(wildcard com_*.h)
user_bin_exe      := bare_am
bare_am           := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
