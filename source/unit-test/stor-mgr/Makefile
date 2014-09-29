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
    fds-vpolicy   \
    fds-hash      \
    fds-migrations \
    fds-am-engine \
    fds-dsk-io    \
    fds-odb       \
    fds-sm-plat   \
    fds-smlib     

user_non_fds_libs :=  \
    config++    \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    crypto \
    leveldb \
    gmock


user_cpp          := \
	object_store_unit_test.cpp \
	object_store_load_test.cpp \
        sm_load_test.cpp \
        metastore_unit_test.cpp   \
        sm_disk_map_test.cpp      \
        sm_token_place_gtest.cpp

user_no_style     := sm_unit_test.cpp
user_bin_exe      := \
    ObjectStoreUnitTest \
    ObjectStoreLoadTest \
    sm_load_test        \
    metastore_unit_test \
    smdiskmap_unit_test \
    smtokenplace_gtest

ObjectStoreUnitTest := object_store_unit_test.cpp
ObjectStoreLoadTest := object_store_load_test.cpp
metastore_unit_test := metastore_unit_test.cpp
smdiskmap_unit_test := sm_disk_map_test.cpp
smtokenplace_gtest  := sm_token_place_gtest.cpp
sm_load_test        := sm_load_test.cpp

include $(topdir)/Makefile.incl
