topdir            := ../..
user_target       := test
user_rtime_env    := user

user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread
user_non_fds_libs := boost_thread boost_system

user_cpp          := ThreadPoolTest.cpp conc_unit_test.cpp fds_actor_ut.cpp
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := threadtest conc_test fds_actor_ut
threadtest        := ThreadPoolTest.cpp
conc_test         := conc_unit_test.cpp
fds_actor_ut      := fds_actor_ut.cpp

include $(topdir)/Makefile.incl
