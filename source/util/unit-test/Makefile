topdir            := ../..
user_target       := test
user_rtime_env    := user

user_fds_so_libs  :=
user_fds_ar_libs  := fds-hash fds-util fds-fdsp
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time Ice IceUtil IceStorm  \
    ssl crypto

# TODO: change the location of fdsp dir
#
user_incl_dir     := ../hash $(topdir)
user_cpp          :=                                                          \
    hash_test.cpp log_unit_test.cpp perfstat_unit_test.cpp

user_bin_exe      := hashtest log_unit_test perfstat_unit_test
hashtest          := hash_test.cpp
log_unit_test     := log_unit_test.cpp
perfstat_unit_test:= perfstat_unit_test.cpp

include $(topdir)/Makefile.incl
