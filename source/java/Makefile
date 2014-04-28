topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_predep       := gen-java/FDS_ProtocolInterface/FDSP_MsgHdrType.java
user_postdep      := compile
user_install      := orchMgr xdiSmokeTest AMAgent
user_install_loc  := exe
user_clean        := target gen-java $(comm_dir_lib)/java $(comm_dir_lib)/admin-webapp

include $(topdir)/Makefile.incl

local_thrift_flags := -r --gen java

compile:
	rm -f $(comm_dir_bin)/AMAgent
	mkdir -p $(comm_dir_lib)/java/
	rm -rf $(comm_dir_lib)/java/classes
	rm -rf gen-java/com
	$(tool_thrift) $(local_thrift_flags) $(topdir)/apis/apis.thrift
	mvn -DskipTests assembly:directory -Dbuild.dir=$(comm_dir_lib)/java
	cp -rv admin-webapp $(comm_dir_lib)

gen-java/FDS_ProtocolInterface/FDSP_MsgHdrType.java: $(topdir)/fdsp/FDSP.thrift
ifdef VERBOSE
	@echo $(tool_thrift) $(local_thrift_flags) $<
	@export LD_LIBRARY_PATH=$(toolchain_lib); $(tool_thrift) $(local_thrift_flags) $<
else
	@echo "    [GEN .thrift]   $<"
	@export LD_LIBRARY_PATH=$(toolchain_lib); $(tool_thrift) $(local_thrift_flags) $<
endif
