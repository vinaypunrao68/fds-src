topdir            := ../..
user_target       := test
user_rtime_env    := user

user_fds_so_libs  :=
user_fds_ar_libs  := fds-hash fds-lib fds-util fds-fdsp fds-thread
user_non_fds_libs :=                                                          \
    boost_log boost_filesystem boost_system boost_thread boost_date_time Ice IceUtil IceStorm  \
    ssl crypto

user_incl_dir     := ../ $(topdir)/include $(topdir)/util
user_cpp_flags    := 
user_cpp          :=                       \
    catalog_unit_test.cpp                  \
    om_client_unit_test.cpp \
    tb_unit_test.cpp \
	qos_dispatcher_unit_test.cpp

user_cc           :=

user_bin_exe      := catalog_test om_client_test tb_unit_test qos_dispatcher_unit_test
catalog_test      := catalog_unit_test.cpp
om_client_test    := om_client_unit_test.cpp
tb_unit_test      := tb_unit_test.cpp
qos_dispatcher_unit_test	:= qos_dispatcher_unit_test.cpp

include $(topdir)/Makefile.incl
