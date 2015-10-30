topdir            := ../../
user_target       := exe 
user_std_libs     := true
user_rtime_env    := user
speedup           = -j $(threads)
user_incl_dir     := $(topdir) ../include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib\
    fds-qos-lib \
    fds-am-engine \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-odb \
    fds-fdsp

user_non_fds_libs := \
    boost_timer \
    leveldb \
    ssl \
    crypto \
    leveldb \
    sqlite3

user_bin_exe      := smchk
smchk             := SMCheckDriver.cpp

include $(topdir)/Makefile.incl
