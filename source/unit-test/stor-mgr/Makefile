test_topdir       := ..
user_build_dir    := svc-test
user_incl_dir     := /usr/include/cryptopp
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=

user_cpp          := \
    object_store_unit_test.cpp \
    object_store_load_test.cpp \
    sm_load_test.cpp \
    metastore_unit_test.cpp   \
    sm_disk_map_test.cpp      \
    sm_token_place_gtest.cpp \
    object_persist_data_gtest.cpp \
    sm_token_compactor_gtest.cpp \
    sm_token_state_gtest.cpp

user_no_style     :=

user_bin_exe      := \
    ObjectStoreUnitTest \
    ObjectStoreLoadTest \
    sm_load_test        \
    metastore_unit_test \
    smdiskmap_unit_test \
    smtokenplace_gtest \
    sm_superblock_gtest \
    obj_persist_gtest \
    smtokcompactor_gtest \
    smtoken_state_gtest

ObjectStoreUnitTest := object_store_unit_test.cpp
ObjectStoreLoadTest := object_store_load_test.cpp
metastore_unit_test := metastore_unit_test.cpp
smdiskmap_unit_test := sm_disk_map_test.cpp
smtokenplace_gtest  := sm_token_place_gtest.cpp
sm_load_test        := sm_load_test.cpp
sm_superblock_gtest := sm_superblock_gtest.cpp
obj_persist_gtest   := object_persist_data_gtest.cpp
smtokcompactor_gtest := sm_token_compactor_gtest.cpp
smtoken_state_gtest  := sm_token_state_gtest.cpp

include $(test_topdir)/Makefile.sm
