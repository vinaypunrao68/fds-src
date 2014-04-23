topdir           := ..
user_target      := lib
user_rtime_env   := user

local_thft_src    := FDSP.thrift orch_proto.thrift
local_pythft_src    := FDSP.thrift
local_thft_hdrdir := $(topdir)/include/fdsp
local_pythft_hdrdir := $(topdir)/test/fdslib/pyfdsp
local_thft_target := $(patsubst %.thrift,$(local_thft_hdrdir)/%_types.h, \
    $(local_thft_src))

# Customize Makefile rules only unique to this directory.
#
local_fdsp_py    := $(local_pythft_hdrdir)/FDS_ProtocolInterface/ttypes.py
user_predep      := $(local_thft_target) $(local_fdsp_py)
user_clean       := \
    $(wildcard $(local_thft_hdrdir)/*.h) \
    $(wildcard $(local_thft_hdrdir)/*.cpp) \
    $(wildcard $(local_pythft_hdrdir)/*) \
    $(local_thft_hdrdir)

include $(topdir)/Makefile.incl

local_thft_flags := --gen cpp:pure_enums --allow-64bit-consts -out $(local_thft_hdrdir)
local_pythft_flags := --gen py -out $(local_pythft_hdrdir) FDSP.thrift

# Rules to build Thrift files
#
define local_mk_thft_cpp
$(patsubst %.thrift,$(local_thft_hdrdir)/%_types.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(tool_thrift) $(local_thft_flags) $$<
	@export LD_LIBRARY_PATH=$(toolchain_lib);                                 \
	$(tool_thrift) $(local_thft_flags) $$<
else
	@echo "    [GEN .thrift]   $$<"
	@export LD_LIBRARY_PATH=$(toolchain_lib);                                 \
	$(tool_thrift) $(local_thft_flags) $$<
endif
endef

$(local_fdsp_py): FDSP.thrift
	$(tool_thrift) $(local_pythft_flags) 

$(foreach thft,$(local_thft_src),$(eval $(call local_mk_thft_cpp,$(thft))))
