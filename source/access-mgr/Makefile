topdir            := ..
user_target       := exe
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    := \
    include \
    amlib \
    connector \
    test

user_incl_dir     := include

user_fds_ar_libs  :=  \
    fds-qos-lib \
    fds-am-engine \
    fds-am-connectors \
    fds-svc-request \
    fds-umod \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-thread \
    fds-osdep

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
    crypto \
    hiredis \
    event \
    rt \
    fiu \
    dl \
    tcmalloc

user_static_libs  := \
    ev

user_cc           :=

user_incl_dir     += $(topdir) \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_cpp      := $(wildcard *.cpp)

#user_ld_flags := -shared -Wl,--no-allow-shlib-undefined

user_no_style     := $(user_cc) $(wildcard com_*.h)
user_bin_exe      := bare_am
bare_am           := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
