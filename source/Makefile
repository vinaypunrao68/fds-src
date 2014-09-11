a:=$(shell [ ! -f /swapfile ] && sudo dd if=/dev/zero of=/swapfile bs=1024000 count=6k && sudo mkswap /swapfile && sudo swapon /swapfile)
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
    fds_client                          \
    am-app                              \
    tools                               \
    java                                \
    unit-test                           \
    checker                             

include $(topdir)/Makefile.incl
