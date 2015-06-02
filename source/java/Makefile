topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

user_thrift       := \
                     $(topdir)/fdsp/FDSP.thrift \
                     $(topdir)/fdsp/fds_stream.thrift \
                     $(topdir)/fdsp/common.thrift \
                     $(topdir)/fdsp/health_monitoring_api.thrift \
                     $(topdir)/fdsp/health_monitoring_types.thrift \
                     $(topdir)/fdsp/am_types.thrift \
                     $(topdir)/fdsp/dm_types.thrift \
                     $(topdir)/fdsp/om_types.thrift \
                     $(topdir)/fdsp/pm_data.thrift \
                     $(topdir)/fdsp/sm_types.thrift \
                     $(topdir)/fdsp/svc_types.thrift \
                     $(topdir)/fdsp/am_api.thrift \
                     $(topdir)/fdsp/config_api.thrift \
                     $(topdir)/fdsp/dm_api.thrift \
                     $(topdir)/fdsp/om_api.thrift \
                     $(topdir)/fdsp/sm_api.thrift \
                     $(topdir)/fdsp/xdi.thrift \
                     $(topdir)/fdsp/svc_api.thrift

user_thrift_flag  := -r --gen java

user_postdep      := compile
user_install      := orchMgr genkey iodriver
user_install_loc  := exe
user_clean        := \
                     target \
                     gen-java \
                     admin-webapp \
                     $(comm_dir_lib)/java \
                     $(comm_dir_lib)/admin-webapp \
                     $(comm_dir_lib)/demo

include $(topdir)/Makefile.incl

#	@rm -rf $(comm_dir_lib)/java

compile:
	@echo "Building Java components"
	@rm -rf $(comm_dir_lib)/java
	@rm -rf $(comm_dir_lib)/admin-webapp
	@mkdir -p $(comm_dir_lib)/java/
#	@mvn install -f thrift-pom.xml --batch-mode
	cd formationds/external/model
	@mvn install -Dmaven.test.skip=true -Dproject.build.dir=$(comm_dir_lib)/java/formationds/external/model -f formationds/external/model/pom.xml --batch-mode
	cd ../../..
	@mvn -Dmaven.test.skip=true assembly:directory -Dproject.build.dir=$(comm_dir_lib)/java --batch-mode
	@mkdir $(comm_dir_lib)/admin-webapp

ifndef SKIP_UI_BUILD
	@cd admin-webapp; npm install; CI=true node_modules/bower/bin/bower --allow-root install; node_modules/grunt-cli/bin/grunt release
	@cp -r admin-webapp/app/* $(comm_dir_lib)/admin-webapp
endif

	@cp -r demo $(comm_dir_lib)
	@cp smokeTest $(main_dir_tools)
	@cp trafficgen $(main_dir_tools)

.PHONY: test

test:
	@mvn -Dproject.build.dir=$(topdir)/cit test --batch-mode
