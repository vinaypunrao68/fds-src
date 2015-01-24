topdir            := ..
user_target       := exe 
user_std_libs     := true
user_rtime_env    := user

user_incl_dir     := \
	. \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
    $(topdir)/access-mgr/include \
	$(topdir)/lib \
    $(topdir)/am-app/include \
	$(topdir)/lib \
	$(topdir)/unit-test/stor-mgr \
	/usr/include/cryptopp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib\
    fds-am-engine \
    fds-am-plat \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-odb \
    fds-vpolicy \
    fds-xdi

user_non_fds_libs := \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    leveldb

user_ar_libs      := smchk
smchk             := smchk.cpp

user_cpp          := checker.cpp dm_checker.cpp smchk.cpp
user_bin_exe      := checker dm_checker smchk
checker		  := checker.cpp
dm_checker    := dm_checker.cpp
smchk         := smchk.cpp smchk_driver.cpp

include $(topdir)/Makefile.incl
