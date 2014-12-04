topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

user_thrift       := $(topdir)/fdsp/FDSP.thrift $(topdir)/apis/apis.thrift $(topdir)/fdsp/fds_stream.thrift $(topdir)/fdsp/snapshot.thrift
user_thrift_flag  := -r --gen java

user_postdep      := compile
user_install      := orchMgr AMAgent genkey
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

ifeq (,$(wildcard *jenkins_built_semaphore))
	@echo "Building Java components"
	@rm -f $(comm_dir_bin)/AMAgent
	@rm -rf $(comm_dir_lib)/java
	@rm -rf $(comm_dir_lib)/admin-webapp
	@mkdir -p $(comm_dir_lib)/java/
	@mvn install -f thrift-pom.xml --batch-mode
	@mvn -Dmaven.test.skip=true assembly:directory -Dproject.build.dir=$(comm_dir_lib)/java --batch-mode
	@mkdir $(comm_dir_lib)/admin-webapp

ifndef SKIP_UI_BUILD
	@cd admin-webapp; npm install; CI=true node_modules/bower/bin/bower --allow-root install; node_modules/grunt-cli/bin/grunt release
	@cp -r admin-webapp/app/* $(comm_dir_lib)/admin-webapp
endif

	@cp -r demo $(comm_dir_lib)
	@cp smokeTest $(main_dir_tools)
	@cp trafficgen $(main_dir_tools)

ifdef JENKINS_URL
	@touch ./jenkins_built_semaphore
endif

else
	@echo "Previous Build under Jenkins succeeded"
endif

.PHONY: test

test:
	@mvn -Dproject.build.dir=$(topdir)/cit test --batch-mode
