/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PLAT_MODULE_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PLAT_MODULE_H_

#include <fds_process.h>
#include "pm_disk_inventory.h"

namespace fds
{
    class FileDiskInventory;

    /**
     * Main module controlling disk/storage HW inventory.
     */

    class DiskPlatModule : public Module
    {
        protected:
            PmDiskInventory::pointer    dsk_devices;
            PmDiskInventory::pointer    dsk_inuse;
            struct udev                *dsk_ctrl;
            struct udev_enumerate      *dsk_enum;
            struct udev_monitor        *dsk_mon;
            FileDiskInventory          *dsk_sim;
            DiskLabelMgr               *label_manager;
            struct pollfd               pollfds[1];

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

            void dsk_rescan();
            void dsk_monitor_hotplug();
            void dsk_commit_label();

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
