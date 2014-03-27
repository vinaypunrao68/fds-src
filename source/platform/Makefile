topdir           := ../
user_target      := exe
user_rtime_env   := user
user_build_dir   := include shared common disk python

user_incl_dir    := include
user_fds_ar_libs := \
    fds-plat-daemon \
	fds-plat-disk \
    fds-plat-lib \
    fds-lib \
    fds-umod \
    fds-util \
    fds-fdsp \
    fds-thread

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    thrift \
    config++ \
	udev \
    hiredis

user_cpp         := platform-main.cpp
user_bin_exe     := platformd
platformd        := $(user_cpp)

include $(topdir)/Makefile.incl
