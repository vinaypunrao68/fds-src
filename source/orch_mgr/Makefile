topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    := orch-cli include om-dlt om-common om-lib om-glue # java
user_install_loc  := exe

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-om-common \
    fds-om-glue \
    fds-om-dlt \
    fds-om-lib \
	fds-plat-lib \
    fds-thread \
    fds-lib \
    fds-util \
    fds-netproxy \
    fds-fdsp \
    fds-umod

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    config++ \
    thrift

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir) include \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

orch_mgr_cpp      :=                   \
    orch-mgr-main.cpp

user_ld_flags := -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm

user_hh           := $(wildcard *.h)
user_cpp          := $(orch_mgr_cpp)
user_bin_exe      := orchMgr
orchMgr           := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
