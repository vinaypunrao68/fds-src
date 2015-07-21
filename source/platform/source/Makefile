topdir         := ../..
user_target    := lib
user_rtime_env := user
user_incl_dir  :=                                \
                  ../include                     \
                  $(topdir)/net-service/include

platform_cpp  :=                        \
                 platform_manager.cpp   \
                 svc_handler.cpp        \
                 valgrind.cpp           \
                 process.cpp

disk_cpp :=                              \
            $(wildcard pm_disk_*.cpp)    \
            $(wildcard file_disk_*.cpp)  \
            $(wildcard disk_*.cpp)


fds_plat_lib_cpp :=                              \
                    platform.cpp                 \
                    domain_cluster_map.cpp       \
                    domain_resources.cpp         \
                    platform_process.cpp         \
                    platform-rpc.cpp             \
                    om_node_agent_evt.cpp        \
                    om_node_agent_evt.cpp        \
                    disk_inventory.cpp           \
                    node_inventory.cpp           \
                    domain_container.cpp         \
                    agent_container.cpp          \
                    node_inv_shmem.cpp           \
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
		    disk_capabilities.cpp	 \
                    fds_flags.cpp

user_cpp := $(platfrom_cpp) $(fds_plat_lib_cpp)
user_ar_libs := fds-pm-lib fds-plat-lib fds-disk-lib
fds-pm-lib   := $(platform_cpp)
fds-plat-lib := $(fds_plat_lib_cpp)
fds-disk-lib := $(disk_cpp)

include $(topdir)/Makefile.incl
