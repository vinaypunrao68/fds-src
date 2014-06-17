topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine

user_non_fds_libs := \
    gmock

user_incl_dir     := \
    $(topdir) \
    $(topdir)/lib \
    $(topdir)/util

user_cpp_flags    := 
user_cpp          :=  \
    fds_timer_test.cpp \
    fds_config_test.cpp \
    catalog_unit_test.cpp \
	ocache_unit_test.cpp         \
    perfstat_unit_test.cpp \
    fds_process_interrupt_test.cpp \
    counters_test.cpp \
    trans_journal_ut.cpp \
    SynchronizedTaskExecutor_ut.cpp

user_cc           :=
user_no_style     := $(user_cpp) $(user_cc) $(wildcard *.h)

user_bin_exe      := \
    fds_timer_test \
    fds_config_test \
    catalog_test \
    ocache_unit_test \
    perfstat_unit_test \
    fds_process_interrupt_test \
    counters_test \
    trans_journal_ut \
    SynchronizedTaskExecutor_ut     

catalog_test               := catalog_unit_test.cpp
#qos_dispatcher_unit_test	:= qos_dispatcher_unit_test.cpp
ocache_unit_test	       := ocache_unit_test.cpp
perfstat_unit_test         := perfstat_unit_test.cpp
fds_process_interrupt_test := fds_process_interrupt_test.cpp
fds_timer_test             := fds_timer_test.cpp
fds_config_test            := fds_config_test.cpp
counters_test              := counters_test.cpp
trans_journal_ut           := trans_journal_ut.cpp
SynchronizedTaskExecutor_ut     := SynchronizedTaskExecutor_ut.cpp

include $(topdir)/Makefile.incl
