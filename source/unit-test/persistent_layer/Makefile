topdir            := ../..
user_target       := test
user_rtime_env    := user

user_incl_dir     :=
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-lib \
    fds-umod \
    fds-thread  \
    fds-util \
    fds-hash \
    fds-fdsp

user_non_fds_libs :=  \
    boost_log \
    boost_system \
    boost_thread  \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    leveldb \
    config++

user_cpp          :=                   \
    dm_service_test.cpp                \
    dm_io_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := dm_svc_test dm_io_test
dm_svc_test       := dm_service_test.cpp
dm_io_test        := dm_io_test.cpp

include $(topdir)/Makefile.incl
