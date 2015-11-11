topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_fds_so_libs  :=
user_fds_ar_libs  := \
    dm-clus-lib \
    fds-test \
    fds-util \
    fds-net-svc \
    fds-svc-request \
    fds-qos-lib \
    fds-lib \
    fds-fdsp \

user_non_fds_libs := \
    gmock \
    curl \
    leveldb \
    pthread \
    jansson

user_incl_dir     := \
    $(topdir)        \
    $(topdir)/lib    \
    $(topdir)/util \
    ./common

user_build_dir    := \
   common 

user_cpp_flags    := 
user_cpp          :=  \
    Volumegroup_gtest.cpp \
    OmMain.cpp \
    DmMain.cpp \
    AmMain.cpp

user_bin_exe      := \
    volumegroup_gtest \
    om \
    dm \
    am

volumegroup_gtest	:= Volumegroup_gtest.cpp
om			:= OmMain.cpp
dm			:= DmMain.cpp
am			:= AmMain.cpp

include $(topdir)/Makefile.incl
