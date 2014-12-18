test_topdir       := ..
user_build_dir    := svc-test
user_incl_dir     := /usr/include/cryptopp

user_hh           := $(wildcard *.h)
user_fds_so_libs  :=
user_fds_ar_libs  :=
user_non_fds_libs := pcrecpp sqlite3
user_cpp_flags    :=
user_cpp          :=  $(wildcard *.cpp)

user_bin_exe      := \
    dm_direct \
    dm_volume_catalog_gtest \
    dm_timelinedb

dm_volume_catalog_gtest     := dm_volume_catalog_gtest.cpp
dm_direct := dm_direct.cpp
dm_timelinedb := timelinedb.cpp

include $(test_topdir)/Makefile.dm
