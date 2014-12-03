topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := $(topdir)/data_mgr/include $(topdir)
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-dm-lib \
	fds-dm-plat \
	fds-plat-lib \
	fds-lib \
        fds-dsk-mgnt \
	fds-net-svc \
	fds-util \
	fds-fdsp \
	fds-umod \
	fds-thread \
	fds-osdep

user_non_fds_libs := \
	boost_log \
	boost_system \
	boost_filesystem \
	boost_thread \
	boost_date_time \
	boost_program_options \
	leveldb thrift  config++ hiredis rt

user_cpp          := blobnode_unit_test.cpp

user_no_style     := $(user_cpp)
user_bin_exe      := blobnode_unit_test
blobnode_unit_test := blobnode_unit_test.cpp

include $(topdir)/Makefile.incl
