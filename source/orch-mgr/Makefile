topdir            := ..
speedup           = -j $(threads)
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
	configservice \
	snapshot \

user_fds_ar_libs  := \
    fds-util \
	fds-net-svc \
	fds-svc-request \
    fds-om-common \
    fds-om-lib \
    fds-om-dlt \
    fds-plat-lib \
    fds-fdsp

user_non_fds_libs := \
    leveldb ical

# TODO: need to move Catalog.h to include, avoid cross module include ref.
#
user_incl_dir     := $(topdir) include \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

orch_mgr_cpp      := $(wildcard *.cpp)

om_output_prefix := lib
om_output_ext := .so
user_ld_flags     := -shared

ifdef BINARY
user_ld_flags     :=
om_output_prefix :=
om_output_ext :=
endif

user_hh           := $(wildcard *.h)
user_no_style     := $(wildcard com_*.h)
user_cpp          := $(orch_mgr_cpp)
user_bin_exe      := $(om_output_prefix)orchmgr$(om_output_ext)
$(om_output_prefix)orchmgr$(om_output_ext)     := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
