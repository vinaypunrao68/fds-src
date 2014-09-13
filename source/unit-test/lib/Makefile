topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-lib \

user_non_fds_libs := \
    gmock

user_incl_dir     := \
    $(topdir)        \
    $(topdir)/lib    \
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
    SynchronizedTaskExecutor_ut.cpp \
    perf_trace_unit_test.cpp \
    kvcache_unit_test.cpp \
    Tracebuffer_ut.cpp \
    smcache_unit_test.cpp \
    object_logger_test.cpp

user_cc           :=
user_no_style     :=                \
    fds_timer_test.cpp              \
    fds_config_test.cpp             \
    catalog_unit_test.cpp           \
    ocache_unit_test.cpp            \
    perfstat_unit_test.cpp          \
    fds_process_interrupt_test.cpp  \
    counters_test.cpp               \
    trans_journal_ut.cpp            \
    SynchronizedTaskExecutor_ut.cpp \
    perf_trace_unit_test.cpp

user_bin_exe      := \
    fds_timer_test \
    fds_config_test \
    catalog_test \
    ocache_unit_test \
    perfstat_unit_test \
    fds_process_interrupt_test \
    counters_test \
    trans_journal_ut \
    SynchronizedTaskExecutor_ut \
    perf_trace_unit_test \
    smcache_unit_test \
    kvcache_unit_test \
    object_logger_test \
    Tracebuffer_ut

catalog_test                   := catalog_unit_test.cpp
ocache_unit_test	       := ocache_unit_test.cpp
perfstat_unit_test             := perfstat_unit_test.cpp
fds_process_interrupt_test     := fds_process_interrupt_test.cpp
fds_timer_test                 := fds_timer_test.cpp
fds_config_test                := fds_config_test.cpp
counters_test                  := counters_test.cpp
trans_journal_ut               := trans_journal_ut.cpp
SynchronizedTaskExecutor_ut    := SynchronizedTaskExecutor_ut.cpp
perf_trace_unit_test           := perf_trace_unit_test.cpp
smcache_unit_test              := smcache_unit_test.cpp
kvcache_unit_test              := kvcache_unit_test.cpp
object_logger_test             := object_logger_test.cpp
Tracebuffer_ut		       := Tracebuffer_ut.cpp

include $(topdir)/Makefile.incl
