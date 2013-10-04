topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build        := unit-test

user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil leveldb

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir)

fds_cli_cpp       := fdscli.cpp
orch_mgr_cpp      :=                   \
    orchMgr.cpp                        \
    OmVolPolicy.cpp

user_cpp          :=                   \
    $(fds_cli_cpp)                     \
    $(orch_mgr_cpp)

user_bin_exe      := fdscli orchMgr
fdscli            := $(fds_cli_cpp)
orchMgr           := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
