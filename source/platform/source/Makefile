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
                     plat-node-workflow.cpp              \
                     disk-capacity.cpp                   \
                     disk-label.cpp                      \
                     disk-partition.cpp                  \
                     disk-inventory.cpp                  \
                     ep-map.cpp                          \
                     net-service.cpp                     \
                     net-platform.cpp                    \
                     net-registration.cpp

# The sources for the public library of platform
fds_plat_lib_cpp :=                              \
                    platform-lib.cpp             \
                    platform-rpc.cpp             \
                    platform-libmod.cpp          \
                    node-inventory.cpp           \
                    shared-disk-inventory.cpp    \
                    node-inv-shmem.cpp           \
                    service-ep-lib.cpp           \
                    BaseAsyncSvcHandler.cpp      \
                    PlatNetSvcHandler.cpp        \
                    node-workflow.cpp            \
                    fds-2phase.cpp               \
                    fds_flags.cpp

user_cpp :=                        \
            $(platform_cpp)        \
            $(platform_core)        \
            $(fds_plat_lib_cpp)

user_ar_libs :=                  \
                platform_core    \
                fds-plat-lib

platform_core := $(platform_core_src)
fds-plat-lib  := $(fds_plat_lib_cpp)

include $(topdir)/Makefile.incl
