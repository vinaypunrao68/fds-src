probe_topdir      := ..
source_topdir     := $(probe_topdir)/../..
user_incl_dir     := \
    $(source_topdir) \
    $(source_topdir)/lib \
    $(source_topdir)/stor-mgr/include \
    $(source_topdir)/stor-mgr/migrations \
    $(source_topdir)/persistent_layer/include \
		/usr/include/cryptopp

user_json         := in.json
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-smlib \
	fds-migrations \
	fds-svc-request \
	fds-net-svc \
	fds-am-engine \
	fds-odb \
	fds-dsk-mgnt \
	fds-sm-plat \
	fds-smlib \
	fds-dsk-io \
	fds-dsk-mgnt \
	fds-odb \
	fds-vpolicy \
	fds-hash \
	fds-migrations \
	fds-net-svc

user_non_fds_libs := \
	boost_timer \
	leveldb \
	ssl \
	cryptopp \
	crypto \
	leveldb

# Don't do style check on auto gen code.
user_hh           := $(wildcard *.h)
user_no_style     := $(patsubst %.json,%.h,$(user_json) adapter.h)

user_cc           := ngx_modules.c
user_cpp          := \
    test-main.cpp \
    adapter.cpp

user_bin_exe      := sm-api
sm-api            := $(user_cpp) $(user_cc)

include $(probe_topdir)/Makefile.svc
