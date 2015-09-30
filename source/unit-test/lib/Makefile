topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-test \
    fds-lib \
    fds-umod \
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
    $(topdir)/util

user_build_dir    := \
    dlt

user_cpp_flags    := 
user_cpp          :=  \
    fds_timer_test.cpp \
    fds_config_test.cpp \
    catalog_unit_test.cpp \
    perfstat_unit_test.cpp \
    fds_process_interrupt_test.cpp \
    counters_test.cpp \
    SynchronizedTaskExecutor_ut.cpp \
    perf_trace_unit_test.cpp \
    kvcache_unit_test.cpp \
    Tracebuffer_ut.cpp \
    sharedcache_perf_test.cpp \
    sharedcache_unit_test.cpp \
    sharedcache_gtest.cpp \
    object_logger_test.cpp \
    HashedLocks_ut.cpp \
    histogram_gtest.cpp \
    qos_tokbucket_gtest.cpp \
    s3utils_gtest.cpp \
    fds_panic.cpp \
	bitset_gtest.cpp \
	rs_container_ut.cpp


user_cc           :=
user_no_style     :=                \
    fds_timer_test.cpp              \
    fds_config_test.cpp             \
    catalog_unit_test.cpp           \
    perfstat_unit_test.cpp          \
    fds_process_interrupt_test.cpp  \
    counters_test.cpp               \
    SynchronizedTaskExecutor_ut.cpp \
    sharedcache_perf_test.cpp       \
    perf_trace_unit_test.cpp \
    rs_container_ut.cpp

user_bin_exe      := \
    fds_timer_test \
    fds_config_test \
    catalog_test \
    perfstat_unit_test \
    fds_process_interrupt_test \
    counters_test \
    perf_trace_unit_test \
    sharedcache_perf_test \
    sharedcache_unit_test \
    sharedcache_gtest \
    kvcache_unit_test \
    object_logger_test \
    Tracebuffer_ut \
    HashedLocks_ut \
    histogram_gtest \
    qos_tokbucket_gtest \
    s3utils_gtest \
    fds_panic \
	bitset_gtest \
	rs_container_ut

catalog_test                   := catalog_unit_test.cpp
perfstat_unit_test             := perfstat_unit_test.cpp
fds_process_interrupt_test     := fds_process_interrupt_test.cpp
fds_timer_test                 := fds_timer_test.cpp
fds_config_test                := fds_config_test.cpp
counters_test                  := counters_test.cpp
SynchronizedTaskExecutor_ut    := SynchronizedTaskExecutor_ut.cpp
perf_trace_unit_test           := perf_trace_unit_test.cpp
sharedcache_perf_test          := sharedcache_perf_test.cpp
sharedcache_unit_test          := sharedcache_unit_test.cpp
sharedcache_gtest	           := sharedcache_gtest.cpp 
kvcache_unit_test              := kvcache_unit_test.cpp
object_logger_test             := object_logger_test.cpp
Tracebuffer_ut		           := Tracebuffer_ut.cpp
HashedLocks_ut		           := HashedLocks_ut.cpp
s3utils_gtest			:= s3utils_gtest.cpp
histogram_gtest 			   := histogram_gtest.cpp
qos_tokbucket_gtest            := qos_tokbucket_gtest.cpp
fds_panic                      := fds_panic.cpp
bitset_gtest				   := bitset_gtest.cpp
rs_container_ut                := rs_container_ut.cpp

include $(topdir)/Makefile.incl
