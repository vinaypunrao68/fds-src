/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <libudev.h>
#include <mntent.h>

#include "disk_plat_module.h"
#include "disk_label_mgr.h"
#include "file_disk_inventory.h"

namespace fds
{
    // -----------------------------------------------------------------------------------
    // Disk Platform Module
    // -----------------------------------------------------------------------------------
    DiskPlatModule    gl_DiskPlatMod("Disk Module");

    DiskPlatModule::DiskPlatModule(char const *const name) : Module(name)
    {
    }
    DiskPlatModule::~DiskPlatModule()
    {
    }

    // dsk_plat_singleton
    // ------------------
    //
    DiskPlatModule *DiskPlatModule::dsk_plat_singleton()
    {
        return &gl_DiskPlatMod;
    }

    // mod_init
    // --------
    //
    int DiskPlatModule::mod_init(SysParams const *const param)
    {
        dsk_devices = new PmDiskInventory();
        dsk_sim     = NULL;
        dsk_inuse   = NULL;
        dsk_ctrl    = udev_new();
        label_manager   = new DiskLabelMgr();
        dsk_enum    = udev_enumerate_new(dsk_ctrl);
        dsk_mon     = udev_monitor_new_from_netlink(dsk_ctrl, "udev");
        fds_assert((dsk_ctrl != NULL) && (dsk_enum != NULL));

        Module::mod_init(param);
        return 0;
    }

    // mod_startup
    // -----------
    //
    void DiskPlatModule::mod_startup()
    {
        Module::mod_startup();
        udev_enumerate_add_match_subsystem(dsk_enum, "block");
        // Add monitor filter for block devices
        udev_monitor_filter_add_match_subsystem_devtype(dsk_mon, "block", NULL);
        // Start the monitoring service. This must happen before the
        // enumeration or there is a chance of lost events.
        udev_monitor_enable_receiving(dsk_mon);
        // Setup the struct that poll will monitor
        pollfds[0].fd = udev_monitor_get_fd(dsk_mon);
        pollfds[0].events = POLLIN;
        pollfds[0].revents = 0;

        dsk_rescan();
        dsk_discover_mount_pts();

        if ((dsk_devices->disk_read_label(label_manager, false) == true) ||
            (dsk_devices->dsk_need_simulation() == false))
        {
            label_manager->clear();
            /* Contains valid disk label or real HW inventory */
            dsk_inuse = dsk_devices;
        }else {
            label_manager->clear();
            const FdsRootDir   *dir = g_fdsprocess->proc_fdsroot();
            dsk_sim   = new FileDiskInventory(dir->dir_dev().c_str());
            dsk_inuse = dsk_sim;
        }

        dsk_inuse->dsk_admit_all();
        dsk_inuse->dsk_mount_all();
        dsk_devices->dsk_dump_all();

        // If in sim mode
        if (dsk_sim != NULL)
        {
            dsk_sim->dsk_dump_all();
        }

        dsk_inuse->disk_read_label(label_manager, true);
    }

    // dsk_discover_mount_pts
    // ----------------------
    //
    void DiskPlatModule::dsk_discover_mount_pts()
    {
        FILE            *mnt;
        struct mntent   *ent;

        mnt = setmntent("/proc/mounts", "r");

        if (mnt == NULL)
        {
            perror("setmnent");
            fds_panic("Can't open /proc/mounts");
        }
        while ((ent = getmntent(mnt)) != NULL)
        {
            if (strstr(ent->mnt_fsname, "/dev/sd") == NULL)
            {
                if (strstr(ent->mnt_fsname, "/dev/xvd") == NULL)
                {
                    continue;
                }
            }

            PmDiskObj::pointer    dsk = dsk_devices->dsk_get_info(ent->mnt_fsname);

            if (dsk == NULL)
            {
                LOGNORMAL << "Can't find maching dev " << ent->mnt_fsname;
            }else {
                dsk->dsk_set_mount_point(ent->mnt_dir);
                dsk->dsk_get_parent()->dsk_set_mount_point(ent->mnt_dir);
            }
            LOGNORMAL << ent->mnt_fsname << " -> " << ent->mnt_dir;
        }
        endmntent(mnt);
    }

    // mod_enable_service
    // ------------------
    //
    void DiskPlatModule::mod_enable_service()
    {
        Module::mod_enable_service();
    }

    // mod_shutdown
    // ------------
    //
    void DiskPlatModule::mod_shutdown()
    {
        if (dsk_sim != NULL)
        {
            delete dsk_sim;
        }
        delete label_manager;
        dsk_devices = NULL;
        dsk_inuse   = NULL;
        udev_enumerate_unref(dsk_enum);
        udev_monitor_unref(dsk_mon);
        udev_unref(dsk_ctrl);
    }

    // dsk_rescan
    // ----------
    //
    void DiskPlatModule::dsk_rescan()
    {
        bool                      blk;
        const char               *path;
        std::string               blk_path;
        std::string               dev_path;
        PmDiskObj::pointer        disk, slice;
        struct udev_list_entry   *devices, *ptr;

        udev_enumerate_scan_devices(dsk_enum);

        devices = udev_enumerate_get_list_entry(dsk_enum);

        dsk_devices->dsk_discovery_begin();

        udev_list_entry_foreach(ptr, devices)
        {
            path = udev_list_entry_get_name(ptr);

            if (PmDiskObj::dsk_blk_dev_path(path, blk_path, dev_path) == false)
            {
                //
                // Only process block/sd and block/vxd devices.
                continue;
            }

            disk = dsk_devices->dsk_get_info(blk_path);

            if (disk == NULL)
            {
                slice = NULL;
            }else {
                slice = dsk_devices->dsk_get_info(blk_path, dev_path);
            }

            if (slice == NULL)
            {
                struct udev_device   *dev;
                Resource::pointer     rs = dsk_devices->rs_alloc_new(ResourceUUID());

                slice = PmDiskObj::dsk_cast_ptr(rs);
                dev   = udev_device_new_from_syspath(dsk_ctrl, path);
                slice->dsk_update_device(dev, disk, blk_path, dev_path);
                dsk_devices->dsk_discovery_add(slice, disk);
            }else {
                // slice->dsk_update_device(NULL, disk, blk_path, dev_path);
                dsk_devices->dsk_discovery_update(slice);
            }
        }

        dsk_devices->dsk_discovery_done();
    }

    // dsk_monitor
    // ---------------
    void DiskPlatModule::dsk_monitor_hotplug()
    {
        // Make the call to poll to determine if there is something to read or not
        while (poll(pollfds, 1, 0) > 0)
        {
            struct udev_device   *dev = udev_monitor_receive_device(dsk_mon);

            if (dev)
            {
                LOGDEBUG << "Received hotplug event: " << udev_device_get_action(dev)
                         << " from device @" << udev_device_get_devnode(dev)
                         << ", " << udev_device_get_subsystem(dev) << ", "
                         << udev_device_get_devtype(dev) << "\n";
                // TODO(brian): determine how we should handle the events (log them for now??)
            }else {
                LOGWARN << "No device from umod_monitor_receive_device. An error occurred.\n";
            }
        }
    }


    // dsk_commit_label
    // ----------------
    //
    void DiskPlatModule::dsk_commit_label()
    {
    }
}  // namespace fds
