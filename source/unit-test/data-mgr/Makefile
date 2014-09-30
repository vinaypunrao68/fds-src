topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-lib \
    fds-dm-plat \
    fds-dm-lib \
    fds-dsk-mgnt \
    fds-xdi

user_non_fds_libs := \
    gmock \
    pthread \
    leveldb \
    crypto \
    cryptopp

user_incl_dir     := \
    $(topdir) \
    $(topdir)/lib \
    $(topdir)/util \
    $(topdir)/data-mgr/include

user_cpp_flags    :=
user_cpp          :=  \
    dm_volume_catalog_gtest.cpp

user_bin_exe      := \
    dm_volume_catalog_gtest

dm_volume_catalog_gtest     := dm_volume_catalog_gtest.cpp

include $(topdir)/Makefile.incl
