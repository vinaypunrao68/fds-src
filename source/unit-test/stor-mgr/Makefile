topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := sm_ut.conf
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib \
    fds-migrations \
    fds-StorHvisor \
    fds-sm-plat \
    fds-plat-lib \
    fds-lib \
    fds-dsk-mgnt \
    fds-umod \
    fds-dsk-io \
    fds-thread \
    fds-util \
    fds-odb \
    fds-vpolicy \
    fds-fdsp \
    fds-hash \
    fds-probe-adapters \
    fds-probe-blk \
    fds-probe 

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    ssl \
    cryptopp \
    leveldb \
    fuse \
    config++ \
    thrift \
    hiredis \
    gmock

user_cpp          := \
	sm_unit_test.cpp \
	SmObjDb_ut.cpp \

user_no_style     := sm_unit_test.cpp
user_bin_exe      := \
	sm_unit_test \
	SmObjDb_ut \

sm_unit_test      := sm_unit_test.cpp
SmObjDb_ut	  := SmObjDb_ut.cpp

include $(topdir)/Makefile.incl
