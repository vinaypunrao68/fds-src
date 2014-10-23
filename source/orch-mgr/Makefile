topdir            := ..
include $(topdir)/Makefile.cpus
speedup           := -j $(parallel_build_count)
user_target       := exe
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := exe
user_build_dir    := \
	orch-cli \
	include \
	om-dlt \
	om-common \
	om-lib \
	om-discovery \
	configservice \
	snapshot \
    om-domain \
	platform

user_fds_ar_libs  := \
    fds-util \
    fds-om-common \
    fds-om-lib \
    fds-om-dlt \
    fds-om-discovery \
    fds-om-plat \
    fds-plat-lib \
    fds-xdi

user_non_fds_libs := \
    leveldb ical

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir) include \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

orch_mgr_cpp      := $(wildcard *.cpp)
user_ld_flags     := -shared

user_hh           := $(wildcard *.h)
user_no_style     := $(wildcard com_*.h)
user_cpp          := $(orch_mgr_cpp)
user_bin_exe      := liborchmgr.so
liborchmgr.so     := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
