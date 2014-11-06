test_topdir       := ../..
user_incl_dir     := .. /usr/include/cryptopp
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    dm_mod_gtest.cpp

user_no_style     :=
user_bin_exe      := \
    dm_mod_gtest

dm_mod_gtest      := dm_mod_gtest.cpp ngx_modules.c

include $(test_topdir)/Makefile.svc
