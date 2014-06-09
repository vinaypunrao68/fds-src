topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user

user_incl_dir     :=
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-dsk-mgnt \
    fds-dsk-io

user_non_fds_libs :=

user_cpp          :=                   \
    dm_service_test.cpp                \
    dm_io_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := dm_io_test
dm_io_test        := dm_io_test.cpp

include $(topdir)/Makefile.incl
