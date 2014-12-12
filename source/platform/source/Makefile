topdir         := ../..
user_target    := lib
user_rtime_env := user
user_incl_dir  :=                              \
                  ../include                   \
                  ../../net-service/include

# The sources for platform_lib, the private library of platform
platform_core_src :=                                     \
                     node_platform.cpp                   \
                     node_platform_proc.cpp              \
                     node_shared_memory_rw_ctrl.cpp      \
                     plat_work_flow.cpp                  \
                     plat_work_item.cpp                  \
                     disk-capacity.cpp                   \
                     disk-label.cpp                      \
                     disk_object.cpp                     \
                     disk_obj_iter.cpp                   \
                     disk-partition.cpp                  \
                     disk_plat_module.cpp                \
                     pm_disk_inventory.cpp               \
                     disk_common.cpp                     \
                     file_disk_obj.cpp                   \
                     pm_disk_obj.cpp                     \
                     file_disk_inventory.cpp             \
                     ep-map.cpp                          \
                     net-service.cpp                     \
                     platform_net_svc.cpp                \
                     net-platform.cpp                    \
                     net-registration.cpp



# The sources for the public library of platform
fds_plat_lib_cpp :=                              \
                    platform-lib.cpp             \
                    platform-rpc.cpp             \
                    platform-libmod.cpp          \
                    disk_inventory.cpp           \
                    node_inventory.cpp           \
                    domain_container.cpp         \
                    agent_container.cpp          \
                    node-inv-shmem.cpp           \
                    pm_svc_ep.cpp                \
                    sm_svc_ep.cpp                \
                    am_svc_ep.cpp                \
                    dm_svc_ep.cpp                \
                    om_svc_ep.cpp                \
                    om_agent.cpp                 \
                    am_agent.cpp                 \
                    sm_agent.cpp                 \
                    dm_agent.cpp                 \
                    pm_agent.cpp                 \
                    node_agent.cpp               \
                    node_services.cpp            \
                    node_agent_evt.cpp           \
                    BaseAsyncSvcHandler.cpp      \
                    PlatNetSvcHandler.cpp        \
                    node_work_flow.cpp           \
                    node_down.cpp                \
                    node_started.cpp             \
                    node_qualify.cpp             \
                    node_upgrade.cpp             \
                    node_work_item.cpp           \
                    node_rollback.cpp            \
                    node_integrate.cpp           \
                    node_deploy.cpp              \
                    node_functional.cpp          \
                    fds-2phase.cpp               \
                    fds_flags.cpp

# # service-ep-lib.cpp           \

user_cpp :=                        \
            $(platform_cpp)        \
            $(platform_core)       \
            $(fds_plat_lib_cpp)

user_ar_libs :=                  \
                platform_core    \
                fds-plat-lib

platform_core := $(platform_core_src)
fds-plat-lib  := $(fds_plat_lib_cpp)

include $(topdir)/Makefile.incl
