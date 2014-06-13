topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      :=
user_install_loc  := test

user_incl_dir     := .. $(topdir)/lib
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-util \
	fds-fdsp \
	fds-umod \
	fds-odb \
	fds-thread \
	fds-hash \
	fds-osdep

user_non_fds_libs := boost_thread \
	boost_log \
	boost_date_time \
	boost_program_options \
	leveldb \
	boost_system \
	leveldb thrift rt config++

user_cpp          := odb_unit_test.cpp
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := odb_unit_test
odb_unit_test     := odb_unit_test.cpp

include $(topdir)/Makefile.incl
