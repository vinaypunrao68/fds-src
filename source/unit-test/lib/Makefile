topdir            := ../..
user_target       := test
user_rtime_env    := user

user_fds_so_libs  :=
user_fds_ar_libs  := fds-hash fds-lib fds-util fds-fdsp fds-thread fds-odb fds-umod
user_non_fds_libs :=                                                          \
    boost_log \
    boost_filesystem \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    Ice IceUtil \
    ssl crypto \
    config++ \
	thrift

user_incl_dir     := $(topdir) $(topdir)/lib $(topdir)/include $(topdir)/util
user_cpp_flags    := 
user_cpp          :=                       \
    fds_timer_test.cpp \
    fds_config_test.cpp \
    catalog_unit_test.cpp \
    om_client_unit_test.cpp \
    tb_unit_test.cpp \
	qos_dispatcher_unit_test.cpp \
	ocache_unit_test.cpp         \
    perfstat_unit_test.cpp \
    fds_process_interrupt_test.cpp

user_cc           :=
user_no_style     := $(user_cpp) $(user_cc) $(wildcard *.h)

user_bin_exe      := \
    fds_timer_test \
    fds_config_test \
    catalog_test \
    om_client_test \
    tb_unit_test \
    qos_dispatcher_unit_test \
    ocache_unit_test \
    perfstat_unit_test\
    fds_process_interrupt_test

catalog_test      := catalog_unit_test.cpp
om_client_test    := om_client_unit_test.cpp
tb_unit_test      := tb_unit_test.cpp
qos_dispatcher_unit_test	:= qos_dispatcher_unit_test.cpp
ocache_unit_test	:= ocache_unit_test.cpp
perfstat_unit_test      := perfstat_unit_test.cpp
fds_process_interrupt_test := fds_process_interrupt_test.cpp
fds_timer_test := fds_timer_test.cpp
fds_config_test:= fds_config_test.cpp

include $(topdir)/Makefile.incl
