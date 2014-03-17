topdir            := ../..
user_target       := exe
user_rtime_env    := user

user_incl_dir     := 
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-util \
	fds-fdsp \
	fds-umod \
	fds-thread \

user_non_fds_libs := boost_log boost_system boost_thread boost_date_time boost_program_options \
    leveldb thrift rt config++ cmdconsole hiredis

user_cpp          :=   configdbtool.cpp
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := configdbtool
configdbtool      := configdbtool.cpp


include $(topdir)/Makefile.incl
