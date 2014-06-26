topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := smlib policy include migrations platform

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) include $(topdir)/stor-mgr/migrations $(topdir)/lib \
                     $(topdir)/persistent_layer/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=                   \
    fds-smlib \
    fds-migrations \
    fds-am-engine \
    fds-odb \
    fds-dsk-mgnt \
    fds-sm-plat \
    fds-plat-lib \
    fds-lib \
    fds-umod \
    fds-dsk-io \
    fds-dsk-mgnt \
    fds-thread \
    fds-util \
    fds-odb \
    fds-vpolicy \
    fds-netproxy \
    fds-fdsp \
    fds-hash \
    fds-migrations \
    fds-osdep

user_non_fds_libs :=                                                          \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time      \
    boost_program_options \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    crypto \
    leveldb \
    config++ \
    thrift \
    hiredis

stor_mgr_cpp      := stormgr_main.cpp
user_cpp          :=                   \
    $(stor_mgr_cpp)

user_no_style     := 
user_bin_exe      := StorMgr
StorMgr           := $(stor_mgr_cpp)

include $(topdir)/Makefile.incl
