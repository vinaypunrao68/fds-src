probe_topdir      := ..
local_topdir      := $(probe_topdir)/../..

user_install_loc  := test
user_install      :=

user_build_dir    := tvc tvc-commit-log vol-cat vol-cat-pl

user_incl_dir     := \
    $(local_topdir)/data-mgr/include \
	$(local_topdir)/access-mgr/include \
    $(local_topdir) \
    $(local_topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
# user_fds_ar_libs  := \
#     fds-dm-lib       \
#     fds-am-engine    \
#     fds-dm-plat      \
#     fds-dm-lib       \
#     fds-plat-lib     \
#     fds-lib          \
#     fds-dsk-mgnt     \
#     fds-dsk-io
user_fds_ar_libs  := \
	 fds-dm-lib      \
	 fds-net-svc     \
     fds-am-engine   \

user_non_fds_libs := \
    leveldb \
    hiredis

user_hh           := $(wildcard *.h)
user_cc           := ngx_modules.c
user_cpp          := \
    mod-main.cpp \
    dm-probe.cpp

user_bin_exe      := dm-probe
dm-probe          := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.s3
