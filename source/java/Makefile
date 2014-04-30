topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

user_thrift       := $(topdir)/fdsp/FDSP.thrift $(topdir)/apis/apis.thrift
user_thrift_flag  := -r --gen java

user_postdep      := compile
user_install      := orchMgr xdiSmokeTest AMAgent
user_install_loc  := exe
user_clean        := \
	target \
	gen-java \
	$(comm_dir_lib)/java \
	$(comm_dir_lib)/admin-webapp

include $(topdir)/Makefile.incl

compile:
	rm -f $(comm_dir_bin)/AMAgent
	mkdir -p $(comm_dir_lib)/java/
	rm -rf $(comm_dir_lib)/java/classes
	rm -rf gen-java/com
	mvn -DskipTests assembly:directory -Dbuild.dir=$(comm_dir_lib)/java
	cp -rv admin-webapp $(comm_dir_lib)
