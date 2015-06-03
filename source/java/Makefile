topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

user_postdep      := compile
user_install      :=
user_install_loc  := exe
user_clean        := \
                     admin-webapp \
                     $(comm_dir_lib)/java \
                     $(comm_dir_lib)/admin-webapp \
                     $(comm_dir_lib)/demo

include $(topdir)/Makefile.incl

compile:
	@echo "Building Java components"

	@mvn clean install -f fds-project/pom.xml --batch-mode -Dmaven.test.skip=false

	@rm -rf $(comm_dir_lib)/java
	@mkdir -p $(comm_dir_lib)/java/

	@cp -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/bin/* $(main_dir_bin)
	@chmod 775 $(main_dir_bin)/*
	@cp -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/tools/* $(main_dir_tools)
	@chmod 775 $(main_dir_tools)/*
	@cp -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/lib/java/* $(comm_dir_lib)/java

ifndef SKIP_UI_BUILD
	@rm -rf $(comm_dir_lib)/admin-webapp
	@mkdir -p $(comm_dir_lib)/admin-webapp
	@cd admin-webapp; npm install; CI=true node_modules/bower/bin/bower --allow-root install; node_modules/grunt-cli/bin/grunt release
	@cp -r admin-webapp/app/* $(comm_dir_lib)/admin-webapp
	@cp -r demo $(comm_dir_lib)
endif

.PHONY: test

test:
	@mvn -Dproject.build.dir=$(topdir)/cit test --batch-mode
