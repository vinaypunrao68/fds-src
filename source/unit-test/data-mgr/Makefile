test_topdir       := ..
user_build_dir    := svc-test
user_incl_dir     :=

user_hh           := $(wildcard *.h)
user_fds_so_libs  :=
user_fds_ar_libs  := fds-qos-lib
user_non_fds_libs := pcrecpp sqlite3  \
    curl \
    jansson
user_cpp_flags    :=

user_bin_exe      := \
    dm_direct \
    dm_volume_catalog_gtest \
    dm_timelinedb \
    archive_gtest \
    ldb_differ_gtest \
    dmchecker_gtest

dm_volume_catalog_gtest     := dm_volume_catalog_gtest.cpp
dm_direct := dm_direct.cpp
dm_timelinedb := timelinedb.cpp
archive_gtest := ArchiveClient_gtest.cpp
ldb_differ_gtest := LeveldbDiffer.cpp LeveldbDiffer_gtest.cpp
dmchecker_gtest := LeveldbDiffer.cpp DmChecker.cpp DmChecker_gtest.cpp 

include $(test_topdir)/Makefile.dm
