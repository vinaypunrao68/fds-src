topdir            := ../../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := 
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-util \
	fds-fdsp \
	fds-umod \
	fds-thread \
	fds-osdep

user_non_fds_libs := \
    boost_log \
	boost_system \
	boost_thread \
	boost_date_time \
	boost_program_options \
    leveldb thrift rt config++

user_cpp          :=                   \
    dlt_unit_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := dlt_unit_test
dlt_unit_test      := dlt_unit_test.cpp
#serialize_unit_test := serialize_ut.cpp
#tt := testthrift.cpp


include $(topdir)/Makefile.incl
