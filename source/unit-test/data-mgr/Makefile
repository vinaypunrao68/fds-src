test_topdir       := ..
topdir            := ../..
user_build_dir    :=
user_incl_dir     := $(topdir)/include/util

user_hh           := $(wildcard *.h)
user_fds_so_libs  :=
user_fds_ar_libs  := \
                    fds-qos-lib \
                    fds-umod \
                    fds-volume-checker
user_non_fds_libs := pcrecpp sqlite3  \
    curl \
    jansson \
    fdsStatsUtil-debug
user_cpp_flags    :=

user_bin_exe      := \
    volumegrouping_gtest \
    dm_direct \
    dm_volume_catalog_gtest \
    dm_timelinedb \
    dm_sequence_ids \
    archive_gtest \
    ldb_differ_gtest \
    dmchecker_gtest \
    objectrefscanner_gtest \
    catalogscanner_gtest


volumegrouping_gtest := VolumeGrouping_gtest.cpp
dm_volume_catalog_gtest     := dm_volume_catalog_gtest.cpp
dm_direct := dm_direct.cpp
dm_timelinedb := timelinedb.cpp
dm_sequence_ids := dm_sequence_ids.cpp

archive_gtest := ArchiveClient_gtest.cpp
ldb_differ_gtest := LeveldbDiffer_gtest.cpp
dmchecker_gtest := DmChecker_gtest.cpp 
objectrefscanner_gtest := ObjectRefScanner_gtest.cpp
catalogscanner_gtest := catalog_scanner_gtest.cpp

include $(test_topdir)/Makefile.dm
