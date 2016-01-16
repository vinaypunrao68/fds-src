/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PLAT_MODULE_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PLAT_MODULE_H_

#include <fds_process.h>
#include "pm_disk_inventory.h"
#include "platform/disk_capabilities.h"

namespace fds
{
    class FileDiskInventory;

    /**
     * Main module controlling disk/storage HW inventory.
     */

    class DiskPlatModule : public Module
    {
        protected:
            static constexpr ssize_t FD_COUNT = 2; // udev and inotify
            static constexpr ssize_t FD_UDEV_IDX = 0;
            static constexpr ssize_t FD_INOTIFY_IDX = 1;

            PmDiskInventory::pointer    dsk_devices;
            PmDiskInventory::pointer    dsk_inuse;
            struct udev                *dsk_ctrl;
            struct udev_enumerate      *dsk_enum;
            struct udev_monitor        *dsk_mon;
            FileDiskInventory          *dsk_sim;
            DiskLabelMgr               *label_manager;
            DiskCapabilitiesMgr        *capabilities_manager;
            struct pollfd               pollfds[FD_COUNT];

            void dsk_discover_mount_pts();

        public:
            explicit DiskPlatModule(char const *const name);
            virtual ~DiskPlatModule();

            static DiskPlatModule *dsk_plat_singleton();

            static inline PmDiskInventory::pointer dsk_get_hdd_inv()
            {
                return dsk_plat_singleton()->dsk_devices;
            }

            static inline PmDiskInventory::pointer dsk_get_ssd_inv()
            {
                return dsk_plat_singleton()->dsk_devices;
            }

            /// The aggregate capacities for storage disks <hdd, sdd>
            std::pair<size_t, size_t> disk_capacities();

            /// The aggregate counts for storage disks <hdd, sdd>
            std::pair<size_t, size_t> disk_counts();

            /// Disk interface type
            fds_disk_type_t disk_type();

            void dsk_rescan();
            void dsk_monitor_hotplug();
            void dsk_commit_label();
            void scan_and_discover_disks();

            // Module methods.
            ///
            virtual int  mod_init(SysParams const *const param);
            virtual void mod_enable_service();
            virtual void mod_startup();
            virtual void mod_shutdown();
    };

    extern DiskPlatModule    gl_DiskPlatMod;

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PLAT_MODULE_H_
