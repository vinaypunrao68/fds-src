topdir         := .
user_ext_build := true
# DO NOT MERGE TO MASTER
##user_predep := use_thrift_from_artifactory

user_build_dir := \
    jansson-2.5   \
    gmock-1.7.0   \
    cmdconsole    \
	gcovr         \
    source

ifeq ($(MAKECMDGOALS),check-optimizations)
	user_rtime_env := user
endif

include $(topdir)/Makefile.incl

.PHONY: use_thrift_from_artifactory

use_thrift_from_artifactory:
	source/Build/mk-scripts/get-thrift.sh;

.PHONY: coverage

all: generate-config-files

coverage:
	cd source && make coverage
