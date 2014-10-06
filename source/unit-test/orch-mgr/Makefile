topdir            := ../..
orch_dir          := $(topdir)/orch_mgr
user_target       := test
user_rtime_env    := user

user_incl_dir     := $(orch_dir)
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-thread \
	fds-util \
	fds-fdsp \
	fds-umod \
    fds-osdep

user_non_fds_libs :=                                                          \
    boost_log \
	boost_system \
	boost_thread \
	boost_date_time \
	boost_program_options \
    leveldb \
	hiredis \
	thrift \
	rt config++ fiu dl

user_cpp          :=                   \
    place_tbl_unit_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := place_tbl_test
place_tbl_test    := place_tbl_unit_test.cpp

include $(topdir)/Makefile.incl
