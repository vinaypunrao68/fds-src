topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib

user_cpp_flags    :=
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
    fds-smlib

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


user_cpp          := \
	object_store_unit_test.cpp \
	object_store_load_test.cpp \
        sm_load_test.cpp \
        metastore_unit_test.cpp   \
        sm_disk_map_test.cpp      \
        sm_token_place_gtest.cpp \
	smapi_gtest.cpp

user_no_style     :=

user_bin_exe      := \
    ObjectStoreUnitTest \
    ObjectStoreLoadTest \
    sm_load_test        \
    metastore_unit_test \
    smdiskmap_unit_test \
    smtokenplace_gtest \
    smapi_gtest \
	sm_superblock_gtest 

ObjectStoreUnitTest := object_store_unit_test.cpp
ObjectStoreLoadTest := object_store_load_test.cpp
metastore_unit_test := metastore_unit_test.cpp
smdiskmap_unit_test := sm_disk_map_test.cpp
smtokenplace_gtest  := sm_token_place_gtest.cpp
smapi_gtest	        := smapi_gtest.cpp ngx_modules.c
sm_load_test        := sm_load_test.cpp
sm_superblock_gtest := sm_superblock_gtest.cpp

include $(topdir)/Makefile.incl
