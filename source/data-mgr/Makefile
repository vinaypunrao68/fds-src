topdir            := ..
user_target       := exe
user_std_libs     := true
speedup           = -j $(threads)
user_rtime_env    := user
user_build_dir    := include dm-lib platform

user_incl_dir     := include $(topdir)
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-dm-lib \
    fds-dm-plat \
    fds-dsk-mgnt \
    fds-net-svc \
    fds-xdi

user_non_fds_libs := \
    leveldb \
    ssl crypto

user_cpp          := dm-main.cpp
user_bin_exe      := DataMgr
DataMgr           := $(user_cpp)

include $(topdir)/Makefile.incl
