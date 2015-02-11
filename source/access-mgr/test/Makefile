topdir            := ../../
user_target       := test
user_install_loc  := test
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    :=

user_incl_dir     := $(topdir) ../include ../stor-hvisor
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-qos-lib \
    fds-am-engine \
    fds-am-connectors \
    fds-am-plat \
    fds-plat-lib \
    fds-net-svc \
    fds-svc-request \
    fds-umod \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-xdi \
    fds-hash \
    fds-thread \
    fds-osdep

user_static_libs  := ev
user_non_fds_libs :=  \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    boost_regex \
    thrift \
    jansson \
    config++ \
    cryptopp \
    gmock \
    hiredis \
    event \
    rt \
    fiu \
    dl \
    profiler \
    tcmalloc

user_cc           :=

user_incl_dir     += $(topdir) \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_cpp      := $(wildcard *.cpp)

user_no_style     := $(user_cc) $(wildcard com_*.h)
user_bin_exe      := AmFunctionalTest NbdFunctionalTest
AmFunctionalTest  := AmFunctionalTest.cpp
NbdFunctionalTest  := NbdFunctionalTest.cpp

include $(topdir)/Makefile.incl
