probe_topdir      := ..
local_thrift_hdr  := thrift-gen
user_build_dir    := $(local_thrift_hdr)
user_incl_dir     := \
    $(local_thrift_hdr)

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-probe-thrift-utest

user_non_fds_libs :=

user_hh           := \
    s3-thrift-probe.h

user_cc           := \
    ngx_modules.c

local_probe_clnt  := \
    s3-thrift-probe.cpp \
    s3-thrift.cpp \
    $(user_cc)

local_server      := \
    thrf-server.cpp

user_cpp          := \
    $(local_server) \
    $(local_probe_clnt)

user_bin_exe      := thrift-probe thrift-server
thrift-probe      := $(local_probe_clnt)
thrift-server     := $(local_server)

# Customize rule for thrift build.
#
local_thrift      := mesg.thrift
local_thrift_tgt  := $(patsubst %.thrift,$(local_thrift_hdr)/%_types.h,$(local_thrift))
local_thrift_flgs := --gen cpp:pure_enums --allow-64bit-consts -out $(local_thrift_hdr)

user_predep       := $(local_thrift_tgt)
user_clean        := $(local_thrift_hdr)

include $(probe_topdir)/Makefile.s3

# Rules to build Thrift files
#
define local_mk_thrift_cpp
$(patsubst %.thrift,$(local_thrift_hdr)/%_types.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(tool_thrift) $(local_thrift_flgs) $$<
	@export LD_LIBRARY_PATH=$(toolchain_lib);                                 \
	$(tool_thrift) $(local_thrift_flgs) $$<
else
	@echo "    [GEN .thrift]   $$<"
	@export LD_LIBRARY_PATH=$(toolchain_lib);                                 \
	$(tool_thrift) $(local_thrift_flgs) $$<
endif
endef

$(foreach thft,$(local_thrift),$(eval $(call local_mk_thrift_cpp,$(thft))))
