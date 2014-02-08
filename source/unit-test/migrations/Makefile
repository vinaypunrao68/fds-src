topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := sender.conf receiver.conf
user_install_loc  := test

user_incl_dir     := $(topdir) $(topdir)/stor-mgr/include $(topdir)/stor-mgr/migrations
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-lib \
    fds-thread \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-probe-adapters \
    fds-probe-blk \
    fds-probe \
    fds-umod \
    fds-migrations

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    ssl \
    crypto \
    leveldb \
    fuse \
    config++ \
    thrift

user_cpp          := migration_ut.cpp 
user_no_style     := $(user_cpp) $(user_cc)

user_bin_exe      := migration_ut 
migration_ut	  := migration_ut.cpp

include $(topdir)/Makefile.incl
