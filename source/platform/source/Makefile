topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := ../include ../../net-service/include

platform_cpp = \
    platform.cpp \
    platform-proc.cpp \
    platform-shm-rw.cpp \
    plat-node-workflow.cpp

disk_lib         := \
    disk-capacity.cpp \
    disk-label.cpp \
    disk-partition.cpp \
    disk-inventory.cpp

net_platform_lib := \
    ep-map.cpp \
    net-service.cpp \
    net-platform.cpp \
    net-registration.cpp

fds_plat_lib_cpp  := \
    platform-lib.cpp \
    platform-rpc.cpp \
    platform-libmod.cpp \
    node-inventory.cpp \
    shared-disk-inventory.cpp \
    node-inv-shmem.cpp \
    service-ep-lib.cpp \
    BaseAsyncSvcHandler.cpp \
    PlatNetSvcHandler.cpp \
    node-workflow.cpp \
    fds-2phase.cpp \
    fds_flags.cpp

user_cpp         := \
    $(platform_cpp) \
    $(disk_lib) \
    $(net_platform_lib) \
    $(fds_plat_lib_cpp)

user_ar_libs     := fds-plat-daemon \
                    fds-plat-disk   \
                    fds-plat-lib

fds-plat-daemon  := $(platform_cpp)       \
                    $(net_platform_lib)

fds-plat-disk    := $(disk_lib)

fds-plat-lib     := $(fds_plat_lib_cpp)

include $(topdir)/Makefile.incl
