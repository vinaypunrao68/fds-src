topdir          := ..
user_target     := exe
user_rtime_env  := user
user_build_dir  := thrift-gen

thrift_odir     := thrift-gen
thrift_flags    := -I . -I $(topdir)/include --gen cpp
thrift_src      := tutorial.thrift shared.thrift

user_incl_dir     := $(thrift_odir)
user_cpp_flags    := -DHAVE_CONFIG_H
user_cpp          := CppClient.cpp CppServer.cpp
user_non_fds_libs := thrift
user_bin_exe      := client server
client            := CppClient.cpp
server            := CppServer.cpp

# Customize Makefile rules only unique to this directory.
#
user_predep       := $(patsubst %.thrift,$(thrift_odir)/%_types.h,$(thrift_src))
user_no_style     := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl

define local_mk_thrift_cpp
$(patsubst %.thrift,$(thrift_odir)/%_types.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(thrift) $(thrift_odir) $$<
	@echo $(tool_thrift) $(thrift_flags) -out $(thrift_odir) $$<
	$(tool_thrift) $(thrift_flags) -out $(thrift_odir) $$<
else
	@echo "    [THRIFT]     $$<"
	@$(tool_thrift) $(thrift_flags) -out $(thrift_odir) $$<
endif
endef

foo:
	@echo "predep: $(user_predep)"

$(foreach thrift,$(thrift_src),$(eval $(call local_mk_thrift_cpp,$(thrift))))
