topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     := $(topdir)/../hiredis
user_fds_so_libs  :=
user_fds_ar_libs  := \
        fds-lib \
        fds-util \
        fds-fdsp \
        fds-thread \
        fds-plat-lib \
	fds-net-svc \
	fds-umod \
	fds-osdep \
	fds-xdi

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    leveldb thrift rt config++ hiredis

user_cpp          := $(wildcard *.cpp)
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := configdb_unit_test
configdb_unit_test   := $(user_cpp)


include $(topdir)/Makefile.incl
