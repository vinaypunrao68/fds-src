topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := sm_ut.conf
user_install_loc  := test

user_incl_dir     := $(topdir) $(topdir)/stor-mgr/include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-smlib \
    fds-lib \
    fds-thread \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-probe-adapters \
    fds-probe-blk \
    fds-probe \
    fds-umod

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    ssl \
    cryptopp \
    leveldb \
    fuse \
    config++ \
    thrift

user_cpp          := sm_unit_test.cpp 
user_no_style     := $(user_cpp)
user_bin_exe      := sm_unit_test 
sm_unit_test      := sm_unit_test.cpp

include $(topdir)/Makefile.incl
