topdir           := ..
user_target      := lib
user_rtime_env   := user

local_thft_src    := FDSP.thrift orch_proto.thrift
local_thft_hdrdir := $(topdir)/include/fdsp
local_thft_target := $(patsubst %.thrift,$(local_thft_hdrdir)/%_types.h, \
    $(local_thft_src))

local_ice_src    := orch_proto.ice
local_ice_outdir := $(topdir)/include/fdsp
local_ice_target := $(patsubst %.ice,$(local_ice_outdir)/%.h, $(local_ice_src))

# Customize Makefile rules only unique to this directory.
#
user_predep      := $(local_ice_target) $(local_thft_target)
user_clean       := \
    $(local_ice_target) \
    $(wildcard $(local_thft_hdrdir)/*.h) \
    $(wildcard $(local_thft_hdrdir)/*.cpp) \
    $(local_thft_hdrdir)

include $(topdir)/Makefile.incl

local_ice_make   := $(toolchain_ice_bin)/slice2cpp
local_ice_flags  := --underscore -I. -I$(topdir)/include
local_ice_flags  += -I$(toolchain_ice)/slice

local_thft_flags := --gen cpp:pure_enums --allow-64bit-consts -out $(local_thft_hdrdir)

# Rules to buld ICE files
#
define local_mk_ice_cpp
$(patsubst %.ice,$(local_ice_outdir)/%.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
	@export LD_LIBRARY_PATH=$(toolchain_ice_lib);                             \
	$(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
else
	@echo "    [ICE-GEN .ice]  $$<"
	@export LD_LIBRARY_PATH=$(toolchain_ice_lib);                             \
	$(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
endif
endef

$(foreach ice,$(local_ice_src),$(eval $(call local_mk_ice_cpp,$(ice))))

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

$(foreach thft,$(local_thft_src),$(eval $(call local_mk_thft_cpp,$(thft))))
