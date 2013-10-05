topdir           := ..
user_target      := lib
user_rtime_env   := user

local_ice_src    := FDSP.ice
local_ice_outdir := $(topdir)/include/fdsp
local_ice_target := $(patsubst %.ice,$(local_ice_outdir)/%.h, $(local_ice_src))

# Customize Makefile rules only unique to this directory.
#
user_predep      := $(local_ice_target)
user_clean       := $(local_ice_target)

include $(topdir)/Makefile.incl

local_ice_make   := $(toolchain_ice_bin)/slice2cpp
local_ice_flags  := --underscore -I. -I$(toolchain_ice)/slice

define local_mk_ice_cpp
$(patsubst %.ice,$(local_ice_outdir)/%.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
	@export LD_LIBRARY_PATH=$(toolchain_ice_lib);                             \
	$(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
else
	@echo "\t[ICE-GEN .ice]\t$$<"
	@export LD_LIBRARY_PATH=$(toolchain_ice_lib);                             \
	$(local_ice_make) $(local_ice_flags) $$< --output-dir $(local_ice_outdir)
endif
endef

$(foreach ice,$(local_ice_src),$(eval $(call local_mk_ice_cpp,$(ice))))
