test_topdir       := ..
user_build_dir    :=
user_incl_dir     :=
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-qos-lib \
    fds-am-engine 

user_cpp          := \
    object_store_unit_test.cpp \
    object_store_load_test.cpp \
    sm_load_test.cpp \
    metastore_unit_test.cpp   \
    sm_disk_map_test.cpp      \
    sm_token_place_gtest.cpp \
    object_persist_data_gtest.cpp \
    sm_token_compactor_gtest.cpp \
    sm_token_state_gtest.cpp     \
    sm_tier_migration_gtest.cpp  \
    sm_token_migration_sequence_gtest.cpp \
    sm_token_migration_double_sequence_gtest.cpp \
    sm_token_migration_track_req_gtest.cpp \
    sm_token_snapshot_delta_gtest.cpp \
    sm_token_persistent_snapshot_gtest.cpp \
    object_metadata_reconcile_gtest.cpp \
    sm_functional_gtest.cpp \
    sm_metadb_gtest.cpp

user_no_style     :=

user_bin_exe      := \
    sm_objectstore_gtest \
    ObjectStoreLoadTest \
    sm_load_test        \
    metastore_unit_test \
    sm_diskmap_unit_test \
    sm_tokenplace_gtest \
    sm_superblock_gtest \
    sm_objdata_persist_gtest \
    sm_tokcompactor_gtest \
    sm_token_state_gtest  \
    sm_tier_migration_gtest \
    sm_token_migration_sequence_gtest \
    sm_token_migration_double_sequence_gtest \
    sm_token_migration_track_req_gtest \
    sm_token_snapshot_delta_gtest \
    sm_token_persistent_snapshot_gtest \
    object_metadata_reconcile_gtest \
    sm_functional_gtest \
    sm_metadb_gtest


sm_objectstore_gtest   := object_store_unit_test.cpp
ObjectStoreLoadTest := object_store_load_test.cpp
metastore_unit_test := metastore_unit_test.cpp
sm_diskmap_unit_test := sm_disk_map_test.cpp
sm_tokenplace_gtest  := sm_token_place_gtest.cpp
sm_load_test        := sm_load_test.cpp
sm_superblock_gtest := sm_superblock_gtest.cpp
sm_objdata_persist_gtest   := object_persist_data_gtest.cpp
sm_tokcompactor_gtest := sm_token_compactor_gtest.cpp
sm_token_state_gtest  := sm_token_state_gtest.cpp
sm_tier_migration_gtest := sm_tier_migration_gtest.cpp
sm_token_migration_sequence_gtest := sm_token_migration_sequence_gtest.cpp 
sm_token_migration_double_sequence_gtest := sm_token_migration_double_sequence_gtest.cpp 
sm_token_migration_track_req_gtest := sm_token_migration_track_req_gtest.cpp 
sm_token_snapshot_delta_gtest := sm_token_snapshot_delta_gtest.cpp
sm_token_persistent_snapshot_gtest := sm_token_persistent_snapshot_gtest.cpp
object_metadata_reconcile_gtest := object_metadata_reconcile_gtest.cpp
sm_functional_gtest := sm_functional_gtest.cpp
sm_metadb_gtest := sm_metadb_gtest.cpp

include $(test_topdir)/Makefile.sm
