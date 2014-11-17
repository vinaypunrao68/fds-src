topdir         := .

# List all directories that builds library first before making exe and unit test
# List one module/line to minimize conflict during merge.
#
# Keep this list sychronized with fastbuild.sh
#
user_build_dir :=                       \
    fdsp                                \
    apis                                \
    include                             \
    osdep                               \
    net-service                         \
    umod-lib                            \
    util                                \
    lib                                 \
    nginx-driver                        \
    fds-probe                           \
    persistent-layer                    \
    platform                            \
    orch-mgr                            \
    data-mgr                            \
    access-mgr                          \
    stor-mgr                            \
    tools                               \
    testlib                             \
    unit-test                           \
    checker                             \
    java

ifdef SKIP_JAVA_BUILD
	user_build_dir := $(filter-out java, $(user_build_dir))
endif

include $(topdir)/Makefile.incl
