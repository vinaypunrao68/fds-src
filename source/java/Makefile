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
	@echo "============================================"
	@echo "-------- Building Java Components ----------"
	@echo "============================================"

	@mvn clean install -f pom.xml -Dosdep.target=$(osdep_target)

	@rm -rf $(comm_dir_lib)/java
	@mkdir -p $(comm_dir_lib)/java/

	@cp -p -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/bin/* $(main_dir_bin)
	@cp -p -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/tools/* $(main_dir_tools)
	@cp -p -r fds-project/fds-dist/target/fds-dist-1.0/fds-dist-1.0/lib/java/* $(comm_dir_lib)/java
	@cp -p -r formationds-project/formationds-external/formationds-ext-dist/target/formationds-ext-dist-*/formationds-ext-dist-*/lib/java/* $(comm_dir_lib)/java

ifndef SKIP_UI_BUILD
	@echo "==========================================="
	@echo "-------- Building GUI Components ----------"
	@echo "==========================================="

	@rm -rf $(comm_dir_lib)/admin-webapp
	@mkdir -p $(comm_dir_lib)/admin-webapp
	@cd admin-webapp; ./nates_awesome_build_script.sh
	@cp -r admin-webapp/app/* $(comm_dir_lib)/admin-webapp
	@cp -r demo $(comm_dir_lib)
endif

.PHONY: test

test:
	@mvn -Dproject.build.dir=$(topdir)/cit test --batch-mode
