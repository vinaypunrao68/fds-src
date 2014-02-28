topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := include dm-lib platform

user_incl_dir     := include $(topdir)
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-dm-lib \
	fds-dm-plat \
    fds-lib \
    fds-dsk-mgnt \
    fds-umod \
    fds-thread \
    fds-util \
	fds-fdsp \

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
	leveldb ssl crypto \
	thrift \
	config++

user_cpp          := dm-main.cpp
user_bin_exe      := DataMgr
DataMgr           := $(user_cpp)

include $(topdir)/Makefile.incl
