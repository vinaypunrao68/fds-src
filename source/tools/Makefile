topdir			  := ..
user_target		  := tool
user_std_libs     := true
user_rtime_env	  := user

user_incl_dir     := \
    $(topdir)/platform/include \
    $(topdir)/net-service/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := 

user_non_fds_libs := 

user_hh           := $(wildcard *.h)
user_cpp          := \
    shm_dump.cpp

user_bin_exe      := shm_dump
shm_dump          := $(user_cpp)

include $(topdir)/Makefile.incl
