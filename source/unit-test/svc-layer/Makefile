test_topdir      := ..
source_dir       := $(test_topdir)/..

user_thrift      := svc.thrift
user_thrift_loc  := gen
user_thrift_flag := --gen cpp:pure_enums --allow-64bit-consts

user_incl_dir     := \
    $(source_dir)/stor-mgr/include \
	$(source_dir)/stor-mgr/migrations \
	$(source_dir)/lib \
	$(source_dir)/include/fdsp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=

user_no_style     :=  \
    $(wildcard gen/*) \
	$(wildcard gen/svc_constants.*) \
	$(wildcard gen/svc_types.*) \
	$(wildcard gen/TestAMSvc.*) \
	$(wildcard gen/TestSMSvc.*) \
	$(wildcard gen/*.skeleton.*)

user_cc           := ngx_modules.c
user_cpp          := \
    th_am.cpp \
    th_sm.cpp \
    gen/svc_constants.cpp \
    gen/svc_types.cpp \
    gen/TestAMSvc.cpp \
    gen/TestSMSvc.cpp \
	threadpool_test.cpp \
    serializer_gtest.cpp

user_predep       := gen_create
user_clean	      := $(user_no_style) $(comm_dir_bin)/gen
user_bin_exe      := \
	th_am \
	th_sm \
	threadpool_test \
    serializer_gtest

th_am := \
    th_am.cpp \
	svc_constants.cpp svc_types.cpp TestAMSvc.cpp TestSMSvc.cpp ngx_modules.c

th_sm := \
    th_sm.cpp \
    svc_constants.cpp svc_types.cpp TestAMSvc.cpp TestSMSvc.cpp ngx_modules.c

threadpool_test := threadpool_test.cpp ngx_modules.c
serializer_gtest := serializer_gtest.cpp ngx_modules.c

include $(test_topdir)/Makefile.svc

gen_create:
	@mkdir -p gen
	@echo $(comm_dir_objs)/gen
	@mkdir -p $(comm_dir_objs)/gen
