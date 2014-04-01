topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := sender.conf receiver.conf migration_ut.conf
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-migrations\
    fds-smlib\
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
    fds-hash

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    ssl \
    crypto \
    leveldb \
    fuse \
    config++ \
    thrift \
    hiredis \
    gmock

user_cpp          := \
	migration_ut.cpp \
	SyncLog_ut.cpp

user_no_style     := migration_ut.cpp

user_bin_exe      := \
	migration_ut \
	SyncLog_ut

migration_ut	  := migration_ut.cpp
SyncLog_ut	  := SyncLog_ut.cpp

include $(topdir)/Makefile.incl
