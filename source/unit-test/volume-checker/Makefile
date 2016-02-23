test_topdir       := ..
user_build_dir    :=
user_incl_dir     := \
                     $(topdir)/util/volume-checker/include

user_hh           := $(wildcard *.h)
user_fds_so_libs  :=
user_fds_ar_libs  := \
                    fds-qos-lib \
                    fds-volume-checker

user_non_fds_libs := pcrecpp sqlite3  \
    curl \
    jansson
user_cpp_flags    :=

user_bin_exe      := volumechecker_gtest 

volumechecker_gtest := VolumeChecker_gtest.cpp

include $(test_topdir)/Makefile.dm
