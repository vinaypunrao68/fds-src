ifeq ($(wildcard ~/.ihaveaswapfile),)
	a:=$(shell [ ! -f /swapfile ] && sudo dd if=/dev/zero of=/swapfile bs=1024000 count=6k)
	bbb := $(shell sudo mkswap /swapfile && sudo swapon /swapfile)
endif

topdir         := .

# List all directories that builds library first before making exe and unit
# test.
# List one module/line to minimize conflict during merge.
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
    fds-probe                           \
    am-engine                           \
    persistent_layer                    \
    platform                            \
    stor-hvisor                         \
    orch_mgr                            \
    data-mgr                            \
    stor-mgr                            \
    access-mgr                          \
    tools                               \
    testlib                             \
    java                                \
    unit-test                           \
    checker                             

ifdef SKIP_JAVA_BUILD
	user_build_dir := $(filter-out java, $(user_build_dir))
endif

include $(topdir)/Makefile.incl
