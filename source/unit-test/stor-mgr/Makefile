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
    fds-smlib     \
    fds-vpolicy   \
    fds-hash      \
    fds-migrations \
    fds-am-engine \
    fds-dsk-io    \
    fds-odb

user_non_fds_libs :=  \
    config++    \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    crypto \
    leveldb


user_cpp          := \
	object_store_unit_test.cpp \
        metastore_unit_test.cpp   

user_no_style     := sm_unit_test.cpp
user_bin_exe      := \
    ObjectStoreUnitTest \
    metastore_unit_test

ObjectStoreUnitTest := object_store_unit_test.cpp
metastore_unit_test := metastore_unit_test.cpp

include $(topdir)/Makefile.incl
