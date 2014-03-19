topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_predep       := gen-java/FDS_ProtocolInterface/FDSP_MsgHdrType.java
user_postdep      := compile
user_install      := orchMgr
user_install_loc  := exe
user_clean        := target gen-java $(comm_dir_lib)/java $(comm_dir_lib)/admin-webapp

include $(topdir)/Makefile.incl

compile:
	mkdir -p $(comm_dir_lib)/java/
	mvn -DskipTests clean assembly:directory -Dbuild.dir=$(comm_dir_lib)/java
	cp -rv admin-webapp $(comm_dir_lib)

#cp -rv target/classes $(comm_dir_lib)/java/
#	cp -rv target/om-1.0-bin/om-1.0/* $(comm_dir_lib)/java/

local_thrift_flags := -r --gen java

gen-java/FDS_ProtocolInterface/FDSP_MsgHdrType.java: $(topdir)/fdsp/FDSP.thrift
ifdef VERBOSE
	@echo $(tool_thrift) $(local_thrift_flags) $<
	@export LD_LIBRARY_PATH=$(toolchain_lib); $(tool_thrift) $(local_thrift_flags) $<
else
	@echo "    [GEN .thrift]   $<"
	@export LD_LIBRARY_PATH=$(toolchain_lib); $(tool_thrift) $(local_thrift_flags) $<
endif
