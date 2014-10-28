test_topdir      := ..
source_dir       := $(test_topdir)/..

user_incl_dir     := \
    $(source_dir)/stor-mgr/include \
	$(source_dir)/stor-mgr/migrations \
	$(source_dir)/lib \
	$(source_dir)/include/fdsp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=

user_non_fds_libs := \
    tcmalloc

user_cc           := ngx_modules.c
user_cpp          := \
	threadpool_test.cpp \
    serializer_gtest.cpp

user_clean	      := 
user_bin_exe      := \
	threadpool_test \
    serializer_gtest

threadpool_test := threadpool_test.cpp ngx_modules.c
serializer_gtest := serializer_gtest.cpp ngx_modules.c

include $(test_topdir)/Makefile.svc

