topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

user_thrift       := $(topdir)/fdsp/FDSP.thrift $(topdir)/apis/apis.thrift $(topdir)/fdsp/fds_stream.thrift $(topdir)/fdsp/snapshot.thrift
user_thrift_flag  := -r --gen java

user_postdep      := compile
user_install      := orchMgr xdiSmokeTest AMAgent genkey
user_install_loc  := exe
user_clean        := \
	target \
	gen-java \
	$(comm_dir_lib)/java \
	$(comm_dir_lib)/admin-webapp \
	$(comm_dir_lib)/demo

include $(topdir)/Makefile.incl

#	@rm -rf $(comm_dir_lib)/java

compile:
	@rm -f $(comm_dir_bin)/AMAgent
	@rm -rf $(comm_dir_lib)/java
	@mkdir -p $(comm_dir_lib)/java/
	@mvn install -f thrift-pom.xml
	@mvn -DskipTests assembly:directory -Dbuild.dir=$(comm_dir_lib)/java
	@mkdir $(comm_dir_lib)/admin-webapp;cd admin-webapp;sudo npm install;bower install;grunt release;
	@cp -r admin-webapp/app/* $(comm_dir_lib)/admin-webapp
	@cp -r demo $(comm_dir_lib)
