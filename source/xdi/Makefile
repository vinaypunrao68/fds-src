topdir           := ..
user_target      := lib
user_rtime_env   := user

local_thft_src    := am_shim.thrift
local_thft_hdrdir := $(topdir)/include/xdi
local_thft_target := $(patsubst %.thrift,$(local_thft_hdrdir)/%_types.h, \
    $(local_thft_src))

# Customize Makefile rules only unique to this directory.
#
user_predep      := $(local_thft_target)
user_clean       := \
    $(wildcard $(local_thft_hdrdir)/*.h) \
    $(wildcard $(local_thft_hdrdir)/*.cpp) \
    $(local_thft_hdrdir)

include $(topdir)/Makefile.incl

local_thft_flags := -r --gen cpp:pure_enums --allow-64bit-consts -out $(local_thft_hdrdir)

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
