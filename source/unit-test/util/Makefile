test_topdir       := ../
topdir            := ../..
util_dir          := $(topdir)/util
user_target       := test
user_rtime_env    := user
user_incl_dir     := include

user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-fdsp \
	fds-hash \
	fds-osdep \
	fds-util

user_non_fds_libs := \
	boost_log \
	boost_system \
	boost_filesystem \
	boost_thread \
	boost_date_time \
    ssl crypto

user_incl_dir     := \
	$(util_dir) \
	$(util_dir)/hash \
	$(topdir)/include/hash

user_cpp          :=  \
    log_unit_test.cpp \
	fds_panic_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := log_unit_test \
                     fds_panic_test bloomtest

log_unit_test     := log_unit_test.cpp
fds_panic_test    := fds_panic_test.cpp
bloomtest         := bloomtest.cpp
include $(test_topdir)/Makefile.svc

