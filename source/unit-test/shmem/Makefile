topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := 
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-umod \
	fds-thread \
	fds-osdep

user_non_fds_libs := \
		boost_thread \
		boost_program_options \
		leveldb thrift rt config++

user_bin_exe      := shm
shm      := shm_unit_test.cpp


include $(topdir)/Makefile.incl
