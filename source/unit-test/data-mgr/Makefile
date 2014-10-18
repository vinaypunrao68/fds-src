topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_hh        := $(wildcard *.h)

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-test \
    fds-probe-ut \
    fds-plat-daemon \
    fds-probe \
    fds-am-engine \
    fds-am-plat \
    fds-dsk-io \
    fds-dsk-mgnt \
    fds-plat-disk \
    fds-hash \
    fds-xdi \
    fds-smlib     \
    fds-vpolicy   \
    fds-hash      \
    fds-migrations \
    fds-odb       \
    fds-sm-plat   \
    fds-smlib \
    fds-dm-plat \
    fds-dm-lib \

user_non_fds_libs := \
    gmock \
    pthread \
    leveldb \
    crypto \
    cryptopp

local_nginx_libs   :=  \
    ngx-http-module \
    ngx-http \
    ngx-event-module \
    ngx-event \
    ngx-core \
    ngx-os \
    ngx-core

user_non_fds_libs :=  \
    jansson \
        udev \
        parted \
    $(local_nginx_libs) \
    config++    \
    boost_timer \
    leveldb \
    ssl \
    crypt \
    pcre \
    crypto \
    z \
    cryptopp \
    leveldb \
    gmock \
    pthread

user_incl_dir     := \
    $(topdir) \
    $(topdir)/lib \
    $(topdir)/util \
    $(topdir)/data-mgr/include

user_cpp_flags    :=
user_cpp          :=  \
    dm_volume_catalog_gtest.cpp \
    dm_mod_gtest.cpp

user_bin_exe      :=  \
    dm_volume_catalog_gtest

dm_volume_catalog_gtest     := dm_volume_catalog_gtest.cpp
dm_mod_gtest                := dm_mod_gtest.cpp

include $(topdir)/Makefile.incl
