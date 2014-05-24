topdir            := ..
user_target       := exe 
user_rtime_env    := user

user_incl_dir     := \
	. \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
    $(topdir)/am-app/include \
	$(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib\
    fds-migrations\
    fds-am-engine \
    fds-am-plat \
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
    fds-probe \
    fds-osdep

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    leveldb \
    fuse \
    config++ \
    thrift \
    hiredis \

user_cpp          := checker.cpp 

user_bin_exe      := checker 
checker		  := checker.cpp

include $(topdir)/Makefile.incl
