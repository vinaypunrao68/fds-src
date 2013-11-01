topdir         := .

# List all directories that builds library first before making exe and unit
# test.
#
user_build_dir :=                                                  \
    fdsp include/fdsp osdep net-proxies umod-lib util lib persistent_layer \
    stor_hvisor_ice \
    umod-lib/test-unit util/unit-test lib/unit-test \
    persistent_layer/unit-test stor_hvisor_ice/unit-test \
    umod-lib/odb/unit-test \
    orch_mgr data_mgr stor_mgr_ice/policy stor_mgr_ice stor_hvisor \
    fds_client

include $(topdir)/Makefile.incl
