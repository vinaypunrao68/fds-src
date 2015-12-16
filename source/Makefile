topdir         := .

# List all directories that builds library first before making exe and unit test
# List one module/line to minimize conflict during merge.
#
# Keep this list sychronized with fastbuild.sh
#
user_build_dir :=                       \
    fdsp                                \
    include                             \
    osdep                               \
    net-service                         \
    umod-lib                            \
    util                                \
    lib                                 \
    persistent-layer                    \
    platform                            \
    orch-mgr                            \
    data-mgr                            \
    access-mgr                          \
    stor-mgr                            \
    tools                               \
    testlib                             \
    unit-test                           \
    java                                \
		packages

ifdef ONLYMAIN
    user_build_dir := $(filter-out unit-test tools java, $(user_build_dir))
endif

# Using DEFER_TESTS=1 on the command line will build the Java directory
# before building the unit-tests.
ifdef DEFER_TESTS
    user_build_dir := $(filter-out unit-test, $(user_build_dir))
    user_build_dir := $(user_build_dir) \
                      unit-test
endif

# Bail out early if we don't have a JAVA_HOME
# This needs to go here, including it later causes make devsetup fail.
ifeq ($(JAVA_HOME),)
    $(error "JAVA_HOME is not configured.  This typically happens during creation of a new \
      development environment (devsetup e.g., ansible).  You can resolve this by running 'source \
      /etc/profile.d/jdk.sh' in your shell and trying your build again.  You can also terminate \
      your shell and login again.")
endif

ifdef SKIP_JAVA_BUILD
    user_build_dir := $(filter-out java, $(user_build_dir))
endif

include $(topdir)/Makefile.incl

install:
	@./dev_make_install.sh

coverage: src_coverage

src_coverage: coverage/source.xml coverage/source.html

coverage/source.xml:
	mkdir -p ../coverage
	../gcovr/gcovr-3.2.build/scripts/gcovr --root="$(shell pwd)" \
	                                       --verbose \
	                                       --object-directory=build.linux-x86_64.debug \
	                                       --relative-objdir \
	                                       --exclude-objdir \
	                                       --html \
	                                       --html-details \
	                                       --output=../coverage/source.html

coverage/source.html:
	mkdir -p ../coverage
	../gcovr/gcovr-3.2.build/scripts/gcovr --root="$(shell pwd)" \
	                                       --verbose \
	                                       --object-directory=build.linux-x86_64.debug \
	                                       --relative-objdir \
	                                       --exclude-objdir \
	                                       --xml \
	                                       --output=../coverage/source.xml
