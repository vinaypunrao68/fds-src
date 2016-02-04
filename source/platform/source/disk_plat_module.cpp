/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <sys/inotify.h>
#include <libudev.h>
#include <mntent.h>

#include "disk_plat_module.h"
#include "disk_label_mgr.h"
#include "file_disk_inventory.h"

namespace fds
{

static constexpr fds_uint32_t MAX_POLL_EVENTS = 1024;
static constexpr fds_uint32_t BUF_LEN = MAX_POLL_EVENTS * (sizeof(struct inotify_event) + NAME_MAX);

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
    capabilities_manager   = new DiskCapabilitiesMgr();
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
    pollfds[FD_UDEV_IDX].fd = udev_monitor_get_fd(dsk_mon);
    pollfds[FD_UDEV_IDX].events = POLLIN;
    pollfds[FD_UDEV_IDX].revents = 0;

    int fd = inotify_init();
    if (fd < 0)
    {
        LOGWARN << "Error initializing inotify " << errno;
    }
    pollfds[FD_INOTIFY_IDX].fd = fd;
    pollfds[FD_INOTIFY_IDX].events = POLLIN;
    pollfds[FD_INOTIFY_IDX].revents = 0;
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    int wd = inotify_add_watch( fd, dir->dir_fds_etc().c_str(), IN_CREATE | IN_OPEN);
    if (wd < 0)
    {
        LOGWARN << "Error adding a watch for inotify " << errno;
    }

    scan_and_discover_disks();
}

fds_uint16_t DiskPlatModule::scan_and_discover_disks()
{
    dsk_rescan();
    dsk_discover_mount_pts();

    if (dsk_devices->dsk_need_simulation() == false)
    {
        /* Contains valid disk label or real HW inventory */
        dsk_inuse = dsk_devices;
    }else {
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

    dsk_inuse->disk_read_capabilities(capabilities_manager);
    dsk_inuse->disk_reconcile_label(label_manager, node_uuid, largest_disk_index);
    label_manager->clear();
    return largest_disk_index;
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
        LOGDEBUG << "checking mount: " << ent->mnt_fsname;
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
            LOGNORMAL << "Can't find matching dev " << ent->mnt_fsname;
        }
        else
        {
            dsk->dsk_set_mount_point(ent->mnt_dir);
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

/// The aggregate capacities for storage disks <hdd, sdd>
std::pair<size_t, size_t> DiskPlatModule::disk_capacities()
{
    return capabilities_manager->disk_capacities();
}

/// The aggregate counts for storage disks <hdd, sdd>
std::pair<size_t, size_t> DiskPlatModule::disk_counts()
{
    return capabilities_manager->disk_counts();
}

/// Disk interface type
fds_disk_type_t DiskPlatModule::disk_type()
{
    // NOTE(bszmyd): Wed 29 Apr 2015 07:11:40 AM MDT
    // Right now we assume all disks have a single interface
    return capabilities_manager->disk_type();
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
    bool fDiskActivationEnabled = g_fdsprocess->get_fds_config()->get<fds_bool_t>("fds.feature_toggle.pm.enable_disk_activation", false);
    LOGNORMAL << "disk activation : " << (fDiskActivationEnabled?"enabled":"disabled");

    dsk_devices->dsk_discovery_begin();

    udev_list_entry_foreach(ptr, devices)
    {
        path = udev_list_entry_get_name(ptr);
        if (fDiskActivationEnabled) {
            struct udev_device   *dev   = udev_device_new_from_syspath(dsk_ctrl, path);

            const char* devName = udev_device_get_property_value(dev, "DEVNAME");
            const char* strFdsDisk = udev_device_get_property_value(dev, "FDS_DISK");

            bool fFdsDisk = (strFdsDisk && *strFdsDisk=='1');

            if (!fFdsDisk) {
                // check the parent..
                struct udev_device   *parent_dev = udev_device_get_parent(dev);
                if (parent_dev) {
                    strFdsDisk = udev_device_get_property_value(parent_dev, "FDS_DISK");
                    fFdsDisk = (strFdsDisk && *strFdsDisk=='1');
                }

                if (fFdsDisk) {
                    GLOGDEBUG << "used parent [" << udev_device_get_property_value(parent_dev, "DEVNAME")
                              << "] to identify [" << devName << "] as fdsdisk";
                }
            }

            if (fFdsDisk) {
                GLOGDEBUG << " fdsdisk identified via udev : " << devName;
            } else {
                GLOGDEBUG << " skipping non fdsdisk : " << devName;
                continue;
            }

        }

        if (PmDiskObj::dsk_blk_dev_path(path, blk_path, dev_path) == false) {
            // Only process block/sd and block/vxd devices.
            continue;
        }
        GLOGDEBUG << "processing : " << path;

        disk = dsk_devices->dsk_get_info(blk_path);

        if (disk == NULL) {
            GLOGDEBUG << "disk is NULL";
            slice = NULL;
        }else {
            slice = dsk_devices->dsk_get_info(blk_path, dev_path);
        }

        if (slice == NULL) {
            GLOGDEBUG << "slice is NULL";
            struct udev_device   *dev;
            dev   = udev_device_new_from_syspath(dsk_ctrl, path);
            if (dev == NULL)
            {
                LOGWARN << "Can't get info about " << path << "; skipping";
            }
            else
            {
                Resource::pointer rs = dsk_devices->rs_alloc_new(ResourceUUID());
                slice = PmDiskObj::dsk_cast_ptr(rs);
                slice->dsk_update_device(dev, disk, blk_path, dev_path);
                dsk_devices->dsk_discovery_add(slice, disk);
            }
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
    char buffer[BUF_LEN] = {0};
    int len;
    // Make the call to poll to determine if there is something to read or not
    while (poll(pollfds, FD_COUNT, -1) > 0)
    {
        bool do_rescan = false;
        if (pollfds[FD_UDEV_IDX].revents > 0)
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
        else if (pollfds[FD_INOTIFY_IDX].revents > 0)
        {
            len = read(pollfds[FD_INOTIFY_IDX].fd, buffer, BUF_LEN);
            if (len < 0)
            {
                LOGWARN << "Failed to read inotify event, error= '" << errno << "'";
            }
            else
            {
                for (char * p = buffer; p < buffer + len; )
                {
                    struct inotify_event * event = reinterpret_cast<struct inotify_event *>(p);
                    if (strcmp(event->name, "adddisk") == 0)
                    {
                        LOGDEBUG << "Received an inotify event " << event->name;
                        do_rescan = true;
                        break;
                    }
                    p += sizeof(struct inotify_event) + event->len;
                }
            }
        }
        else
        { // should never happen
            LOGWARN << "poll call returned with no event";
        }

        if (do_rescan)
        {
            return; // return to caller, so it knows a rescan needs to happen
        }
    }
}

void DiskPlatModule::set_node_uuid(NodeUuid uuid)
{
    node_uuid = uuid;
}

void DiskPlatModule::set_largest_disk_index(fds_uint16_t disk_index)
{
    largest_disk_index = disk_index;
}

fds_uint16_t DiskPlatModule::get_largest_disk_index()
{
    return largest_disk_index;
}

// dsk_commit_label
// ----------------
//
void DiskPlatModule::dsk_commit_label()
{
}
}  // namespace fds
