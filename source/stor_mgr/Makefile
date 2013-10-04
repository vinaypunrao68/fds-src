topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build        := unit-test
user_fds_so_libs  :=
user_fds_ar_ligs  := fds-util
user_non_fds_libs := leveldb
user_cpp          :=                 \
    stor_mgr.cpp                     \
    disk_mgr.cpp                     \
    odb.cpp

user_bin_exe     := stormgr
stormgr          := $(user_cpp)

include $(topdir)/Makefile.incl
