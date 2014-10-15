topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_thrift      := svc.thrift
user_thrift_loc  := . 
user_thrift_flag := --gen cpp:pure_enums --allow-64bit-consts

user_incl_dir     := \
	. \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib \
	$(topdir)/include/fdsp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-test \
    fds-probe-ut \
    fds-plat-daemon \
    fds-probe \
    fds-am-engine \
    fds-am-plat \
    fds-dsk-io \
    fds-dsk-mgnt \
    fds-plat-disk \
    fds-hash \
    fds-xdi \
    fds-smlib     \
    fds-vpolicy   \
    fds-hash      \
    fds-migrations \
    fds-odb       \
    fds-sm-plat   \
    fds-smlib

local_nginx_libs   :=  \
    ngx-http-module \
    ngx-http \
    ngx-event-module \
    ngx-event \
    ngx-core \
    ngx-os \
    ngx-core

user_non_fds_libs :=  \
    jansson \
	udev \
	parted \
    $(local_nginx_libs) \
    config++    \
    boost_timer \
    leveldb \
    ssl \
    crypt \
    pcre \
    crypto \
    z \
    cryptopp \
    leveldb \
    gmock \
    profiler \
    pthread

user_no_style     :=  \
	$(wildcard svc_constants.*) \
	$(wildcard svc_types.*) \
	$(wildcard TestAMSvc.*) \
	$(wildcard TestSMSvc.*) \
	$(wildcard *.skeleton.*)

user_cpp          := \
	th_am.cpp \
	th_sm.cpp \
	threadpool_test.cpp \
    serializer_gtest.cpp

user_clean	  := \
	$(wildcard svc_constants.*) \
	$(wildcard svc_types.*) \
	$(wildcard TestAMSvc.*) \
	$(wildcard TestSMSvc.*) \
	$(wildcard *.skeleton.*)

user_bin_exe      := \
	th_am \
	th_sm \
	threadpool_test \
    serializer_gtest

th_am := th_am.cpp svc_constants.cpp svc_types.cpp TestAMSvc.cpp TestSMSvc.cpp ngx_modules.c
th_sm := th_sm.cpp svc_constants.cpp svc_types.cpp TestAMSvc.cpp TestSMSvc.cpp ngx_modules.c
threadpool_test := threadpool_test.cpp ngx_modules.c
serializer_gtest := serializer_gtest.cpp ngx_modules.c

include $(topdir)/Makefile.incl
