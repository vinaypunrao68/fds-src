test_topdir       := ../..
user_incl_dir     := /usr/include/cryptopp
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-qos-lib \
    fds-am-engine \

user_cc           := ngx_modules.c
user_cpp          := \
    smapi_gtest.cpp

user_no_style     :=
user_bin_exe      := \
    smapi_gtest \
    smperf_gtest \
    smperf_dmsm_gtest \
    slsmperf_gtest \
    svc_gtest

smapi_gtest           := smapi_gtest.cpp ngx_modules.c
smperf_gtest          := smperf_gtest.cpp ngx_modules.c
slsmperf_gtest        := slsmperf_gtest.cpp ngx_modules.c
svc_gtest             := svc_gtest.cpp ngx_modules.c
smperf_dmsm_gtest     := smperf_dmsm_gtest.cpp ngx_modules.c

include $(test_topdir)/Makefile.svc
