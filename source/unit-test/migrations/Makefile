topdir            := ../..
user_target       := test
user_rtime_env    := user
user_std_libs     := true
user_install      := sender.conf receiver.conf migration_ut.conf
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib \
	/usr/include/cryptopp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-migrations \
    fds-smlib \
    fds-qos-lib \
    fds-am-engine \
    fds-sm-plat \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-odb \
    fds-vpolicy \
    fds-xdi

user_non_fds_libs := \
    leveldb \
    ssl \
    cryptopp \
    crypto \
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
