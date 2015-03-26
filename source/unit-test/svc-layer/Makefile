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
    tcmalloc \
	pthread \
    event

user_cc           :=
user_cpp          := \
    th_am.cpp \
    th_sm.cpp \
    th_dm.cpp \
	threadpool_test.cpp \
    serializer_gtest.cpp

user_clean	      := 
user_bin_exe      := \
	th_am \
	th_sm \
    th_dm \
    th_bare \
	threadpool_test \
    serializer_gtest

th_am := \
    th_am.cpp

th_sm := \
    th_sm.cpp

th_dm := \
    th_dm.cpp

th_bare := \
	th_bare.cpp

#th_am_server := \
#    th_am_server.cpp

threadpool_test := threadpool_test.cpp
serializer_gtest := serializer_gtest.cpp

include $(test_topdir)/Makefile.svc
