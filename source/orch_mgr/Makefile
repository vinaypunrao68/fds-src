topdir            := ..
user_target       := exe
user_rtime_env    := user
user_install_loc  := exe
user_build_dir    := \
	orch-cli \
	include \
	om-dlt \
	om-common \
	om-lib \
	om-discovery \
	configservice

user_fds_ar_libs  := \
    fds-om-common \
    fds-om-dlt \
    fds-om-lib \
    fds-om-discovery \
    fds-plat-lib \
    fds-lib \
    fds-util \
    fds-netproxy \
    fds-xdi \
    fds-fdsp \
    fds-umod \
    fds-thread

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    config++ \
    hiredis \
    thrift

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir) include \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

orch_mgr_cpp      := $(wildcard *.cpp)

user_ld_flags := -shared

user_hh           := $(wildcard *.h)
user_no_style     := $(wildcard com_*.h)
user_cpp          := $(orch_mgr_cpp)
user_bin_exe      := liborchmgr.so
liborchmgr.so     := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
