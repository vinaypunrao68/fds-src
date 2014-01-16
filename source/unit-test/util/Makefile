topdir            := ../..
util_dir          := $(topdir)/util
user_target       := test
user_rtime_env    := user

user_fds_so_libs  :=
user_fds_ar_libs  := fds-hash fds-util fds-fdsp
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_filesystem boost_thread boost_date_time Ice IceUtil \
    ssl crypto

user_incl_dir     := $(util_dir) $(util_dir)/hash $(topdir)/include/hash
user_cpp          :=                                                          \
    hash_test.cpp log_unit_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := hashtest log_unit_test 
hashtest          := hash_test.cpp
log_unit_test     := log_unit_test.cpp

include $(topdir)/Makefile.incl
