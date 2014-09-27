/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>
#include <string>
#include <fds_uuid.h>
#include <fds_process.h>
#include <disk-label.h>
#include <disk-partition.h>

namespace fds {

DiskCommon::DiskCommon(const std::string &blk) : dsk_blk_path(blk) {}
DiskCommon::~DiskCommon() {}

PmDiskObj::~PmDiskObj()
{
    if (dsk_my_dev != NULL) {
        udev_device_unref(dsk_my_dev);
        dsk_my_dev = NULL;
    }
    if (dsk_parent != NULL) {
        dsk_parent->rs_mutex()->lock();
        dsk_part_link.chain_rm_init();
        fds_verify(dsk_common == dsk_parent->dsk_common);
        dsk_parent->rs_mutex()->unlock();

        dsk_common = NULL;
        dsk_parent = NULL;  // free parent obj when refcnt = 0.
    } else {
        if (dsk_common != NULL) {
            delete dsk_common;
        }
    }
    fds_verify(dsk_label == NULL);
    fds_verify(dsk_part_link.chain_empty());
    fds_verify(dsk_disc_link.chain_empty());
    fds_verify(dsk_type_link.chain_empty());
    fds_verify(dsk_part_head.chain_empty_list());

    LOGNORMAL << "free " << rs_name;
}

PmDiskObj::PmDiskObj()
    : dsk_part_link(this), dsk_disc_link(this), dsk_raw_path(NULL), dsk_parent(NULL),
      dsk_my_dev(NULL), dsk_common(NULL), dsk_label(NULL)
{
    rs_name[0]   = '\0';
    dsk_cap_gb   = 0;
    dsk_part_idx = 0;
    dsk_part_cnt = 0;
    dsk_my_devno = 0;
    dsk_raw_plen = 0;
}

// dsk_set_mount_point
// -------------------
//
void
PmDiskObj::dsk_set_mount_point(const char *mnt)
{
    dsk_mount_pt.assign(mnt);
}

// dsk_get_mount_point
// -------------------
//
const std::string &
PmDiskObj::dsk_get_mount_point()
{
    if (dsk_mount_pt.empty()) {
        if (DiskPlatModule::dsk_get_hdd_inv()->dsk_need_simulation() == true) {
            // In simulation, the mount point is the same as the device.
            dsk_mount_pt.assign(rs_name);
        }
    }
    return dsk_mount_pt;
}

// dsk_update_device
// -----------------
//
void
PmDiskObj::dsk_update_device(struct udev_device *dev,
                             PmDiskObj::pointer  ref,
                             const std::string  &blk_path,
                             const std::string  &dev_path)
{
    if (dev == NULL) {
        dev = dsk_my_dev;
    } else {
        fds_assert(dsk_my_dev == NULL);
        fds_assert(dsk_parent == NULL);
        dsk_my_dev = dev;
    }
    dsk_cap_gb   = strtoull(udev_device_get_sysattr_value(dev, "size"), NULL, 10);
    dsk_cap_gb   = dsk_cap_gb >> (30 - 9);
    dsk_my_devno = udev_device_get_devnum(dev);
    dsk_raw_path = udev_device_get_devpath(dev);
    dsk_raw_plen = strlen(dsk_raw_path);
    strncpy(rs_name, dev_path.c_str(), sizeof(rs_name));

    if (ref == NULL) {
        dsk_parent = this;
        dsk_common = new DiskCommon(blk_path);
    } else {
        fds_assert(ref->dsk_common != NULL);
        fds_assert(ref->dsk_common->dsk_blk_path == blk_path);
        dsk_fixup_family_links(ref);
    }
    if (dsk_parent == this) {
        dsk_read_uuid();
    } else {
        /*  Generate uuid for this partition slide, don't have to be persistent. */
        rs_uuid = fds_get_uuid64(get_uuid());
    }
}

// dsk_get_info
// ------------
//
PmDiskObj::pointer
PmDiskObj::dsk_get_info(const std::string &dev_path)
{
    ChainList          *slices;
    ChainIter           iter;
    PmDiskObj::pointer  found;

    found = NULL;
    if (dsk_parent != NULL) {
        fds_verify(dsk_parent->dsk_parent == dsk_parent);
        fds_assert(dsk_parent->dsk_common == dsk_common);

        dsk_parent->rs_mutex()->lock();
        slices = &dsk_parent->dsk_part_head;
        chain_foreach(slices, iter) {
            found = slices->chain_iter_current<PmDiskObj>(iter);
            fds_verify(found->dsk_parent == dsk_parent);
            fds_verify(found->dsk_common == dsk_common);

            if (strncmp(found->rs_name, dev_path.c_str(), sizeof(found->rs_name)) != 0) {
                found = NULL;
                continue;
            }
            break;
        }
        dsk_parent->rs_mutex()->unlock();
    }
    return found;
}

// dsk_read_uuid
// -------------
//
void
PmDiskObj::dsk_read_uuid()
{
    fds_uint64_t  uuid;
    DiskLabel    *tmp;

    if (dsk_label == NULL) {
        tmp = new DiskLabel(this);
        rs_mutex()->lock();
        if (dsk_label == NULL) {
            dsk_label = tmp;
        } else {
            delete tmp;
        }
        rs_mutex()->unlock();
    }
    dsk_label->dsk_label_read();

    rs_mutex()->lock();
    rs_uuid = dsk_label->dsk_label_my_uuid();
    if (rs_uuid.uuid_get_val() == 0) {
        uuid    = fds_get_uuid64(get_uuid());
        rs_uuid = ResourceUUID(uuid);
        dsk_label->dsk_label_save_my_uuid(rs_uuid);
    } else {
        LOGNORMAL << "Read uuid " << std::hex << rs_uuid.uuid_get_val()
            << " from " << rs_name << std::dec;
    }
    rs_mutex()->unlock();
}

// dsk_dev_foreach
// ---------------
//
void
PmDiskObj::dsk_dev_foreach(DiskObjIter *iter, bool parent_only)
{
    if ((dsk_parent == NULL) || ((parent_only == true) && (dsk_parent == this))) {
        return;
    }
    int           cnt;
    ChainIter     i;
    ChainList    *l;
    DiskObjArray  disks(dsk_parent->dsk_part_cnt << 1);

    cnt = 0;
    dsk_parent->rs_mutex()->lock();
    l = &dsk_parent->dsk_part_head;
    chain_foreach(l, i) {
        disks[cnt++] = l->chain_iter_current<PmDiskObj>(i);
    }
    dsk_parent->rs_mutex()->unlock();

    for (int j = 0; j < cnt; j++) {
        if (iter->dsk_iter_fn(disks[j]) == false) {
            break;
        }
    }
}

// dsk_fixup_family_links
// ----------------------
//
void
PmDiskObj::dsk_fixup_family_links(PmDiskObj::pointer ref)
{
    ChainIter           iter;
    ChainList          *slices, tmp;
    PmDiskObj::pointer  parent, disk;

    parent = ref->dsk_parent;
    fds_assert(parent != NULL);

    // The shortest raw path is the parent object.
    if (dsk_raw_plen < parent->dsk_raw_plen) {
        parent = this;

        // Need to reparent all devices to this obj.
        ref->rs_mutex()->lock();
        slices = &ref->dsk_part_head;
        chain_foreach(slices, iter) {
            disk = slices->chain_iter_current<PmDiskObj>(iter);
            fds_verify(disk->dsk_parent == parent);

            disk->dsk_parent = this;
        }
        tmp.chain_transfer(slices);
        ref->rs_mutex()->unlock();

        rs_mutex()->lock();
        dsk_part_head.chain_transfer(&tmp);
        rs_mutex()->unlock();
        fds_verify(ref->dsk_parent == this);
    }
    dsk_parent = parent;
    dsk_common = parent->dsk_common;
    fds_verify(dsk_common != NULL);

    if (dsk_part_link.chain_empty()) {
        parent->rs_mutex()->lock();
        parent->dsk_part_cnt++;
        parent->dsk_part_head.chain_add_back(&dsk_part_link);
        parent->rs_mutex()->unlock();
    }
}

class DiskPrintIter : public DiskObjIter
{
  public:
    bool dsk_iter_fn(DiskObj::pointer disk) {
        LOGNORMAL << PmDiskObj::dsk_cast_ptr(disk);
        return true;
    }
};

std::ostream &operator<< (std::ostream &os, PmDiskObj::pointer obj)
{
    ChainIter   i;
    ChainList  *l;

    if (obj->dsk_parent == obj) {
        os << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec
           << " - " << obj->dsk_cap_gb << " GB]\n";
        os << obj->dsk_common->dsk_get_blk_path() << std::endl;
        os << obj->dsk_raw_path << std::endl;

        DiskPrintIter iter;
        obj->dsk_dev_foreach(&iter);
    } else {
        os << "  " << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec
           << " - " << obj->dsk_cap_gb << " GB]\n";
        if (obj->dsk_raw_path != NULL) {
            os << "  " << obj->dsk_raw_path << "\n";
        }
    }
    os << std::endl;
    return os;
}

// dsk_blk_dev_path
// ----------------
//
bool
PmDiskObj::dsk_blk_dev_path(const char *raw, std::string *blk, std::string *dev)
{
    const char *block;

    block = strstr(raw, "block/sd");
    if (block == NULL) {
        block = strstr(raw, "block/xvd");
        if (block != NULL) { goto ok; }
        return false;
    }
 ok:
    blk->assign(raw, (size_t)(block - raw) + sizeof("block"));
    dev->assign("/dev");

    // Goto the end and go back until we hit the '/'
    for (block += sizeof("block/"); *block != '\0'; block++) {
    }
    for (block--; *block != '/'; block--) {
        fds_verify(block > raw);
    }
    // Append the device name to the output string.
    dev->append(block);
    return true;
}

// dsk_read
// --------
//
int
PmDiskObj::dsk_read(void *buf, fds_uint32_t sector, int sec_cnt)
{
    int fd, rt;

    fd = open(rs_name, O_RDWR | O_SYNC);
    rt = pread(fd, buf,
               fds_disk_sector_to_byte(sec_cnt), fds_disk_sector_to_byte(sector));
    close(fd);
    return rt;
}

// dsk_write
// ---------
//
int
PmDiskObj::dsk_write(bool sim, void *buf, fds_uint32_t sector, int sec_cnt)
{
    int fd, rt;

    /* Don't touch sda device */
    if (sim == true) {
        LOGNORMAL << "Skiping real disk in sim env..." << rs_name;
        return 0;
    }
    fds_verify((sector + sec_cnt) <= 16384);  // TODO(Vy): no hardcode
    fd = open(rs_name, O_RDWR | O_SYNC);
    rt = pwrite(fd, buf,
                fds_disk_sector_to_byte(sec_cnt), fds_disk_sector_to_byte(sector));
    close(fd);
    if (rt < 0) {
        perror("Error");
    }
    LOGNORMAL << "Write supperblock to " << rs_name << ", sector " << sector
            << ", ret " << rt << ", sect cnt " << sec_cnt;
    return rt;
}

// -------------------------------------------------------------------------------------
// PM Disk Inventory
// -------------------------------------------------------------------------------------
PmDiskInventory::PmDiskInventory() : DiskInventory(), dsk_qualify_cnt(0)
{
    dsk_partition = new DiskPartitionMgr();
}

PmDiskInventory::~PmDiskInventory()
{
    delete dsk_partition;

    // Setup conditions as if all disks are gone after the discovery.
    dsk_prev_inv.chain_transfer(&dsk_curr_inv);
    dsk_prev_inv.chain_transfer(&dsk_discovery);
    dsk_discovery_done();
}

// rs_new
// ------
//
Resource *
PmDiskInventory::rs_new(const ResourceUUID &uuid)
{
    return new PmDiskObj();
}

// dsk_get_info
// ------------
//
PmDiskObj::pointer
PmDiskInventory::dsk_get_info(dev_t dev_node)
{
    return NULL;
}

PmDiskObj::pointer
PmDiskInventory::dsk_get_info(dev_t dev_node, int partition)
{
    return NULL;
}

PmDiskObj::pointer
PmDiskInventory::dsk_get_info(const std::string &blk_path)
{
    PmDiskObj::pointer disk;

    rs_mtx.lock();
    disk = dsk_dev_map[blk_path];
    rs_mtx.unlock();

    return disk;
}

PmDiskObj::pointer
PmDiskInventory::dsk_get_info(const std::string &blk_path, const std::string &dev_path)
{
    PmDiskObj::pointer disk;

    disk = dsk_get_info(blk_path);
    if (disk != NULL) {
        return disk->dsk_get_info(dev_path);
    }
    return NULL;
}

// dsk_discovery_begin
// -------------------
//
void
PmDiskInventory::dsk_discovery_begin()
{
    LOGNORMAL << "Begin disk discovery...";
    rs_mtx.lock();
    if (dsk_prev_inv.chain_empty_list() && dsk_discovery.chain_empty_list()) {
        dsk_prev_inv.chain_transfer(&dsk_curr_inv);
        fds_assert(dsk_curr_inv.chain_empty_list());
    }
    dsk_qualify_cnt = 0;
    rs_mtx.unlock();
}

// dsk_discovery_add
// -----------------
//
void
PmDiskInventory::dsk_discovery_add(PmDiskObj::pointer disk, PmDiskObj::pointer ref)
{
    fds_verify(disk->dsk_common != NULL);
    rs_mtx.lock();

    // Add to the map indexed by full HW device path.
    if (ref == NULL) {
        dsk_dev_map[disk->dsk_common->dsk_blk_path] = disk;
    } else {
        fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] == ref->dsk_parent);
    }
    dsk_discovery.chain_add_back(&disk->dsk_disc_link);
    if (disk->dsk_parent == disk) {
        DiskInventory::dsk_add_to_inventory_mtx(disk, &dsk_hdd);
        if (disk->dsk_capacity_gb() >= DISK_ALPHA_CAPACITY_GB) {
            dsk_qualify_cnt++;
        }
    } else {
        DiskInventory::dsk_add_to_inventory_mtx(disk, NULL);
    }
    rs_mtx.unlock();
}

// dsk_discovery_update
// --------------------
//
void
PmDiskInventory::dsk_discovery_update(PmDiskObj::pointer disk)
{
    fds_verify(disk->dsk_common != NULL);
    rs_mtx.lock();
    fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);

    // Rechain this obj to the discovery list.
    disk->dsk_disc_link.chain_rm();
    dsk_discovery.chain_add_back(&disk->dsk_disc_link);
    rs_mtx.unlock();
}

// dsk_discovery_remove
// --------------------
//
void
PmDiskInventory::dsk_discovery_remove(PmDiskObj::pointer disk)
{
    rs_mtx.lock();

    // Unlink the chain and unmap every indices.
    disk->dsk_disc_link.chain_rm_init();
    if (disk->dsk_parent == disk) {
        fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);
        dsk_dev_map.erase(disk->dsk_common->dsk_blk_path);
    }
    DiskInventory::dsk_remove_out_inventory_mtx(disk);
    rs_mtx.unlock();

    // Break free from the inventory.
    rs_free_resource(disk);
}

// dsk_discovery_done
// ------------------
//
void
PmDiskInventory::dsk_discovery_done()
{
    ChainList           rm;
    PmDiskObj::pointer disk;

    rs_mtx.lock();
    // Anything left over in the prev inventory list are phantom devices.
    dsk_curr_inv.chain_transfer(&dsk_discovery);
    rm.chain_transfer(&dsk_prev_inv);

    fds_assert(dsk_prev_inv.chain_empty_list());
    fds_assert(dsk_discovery.chain_empty_list());
    rs_mtx.unlock();

    while (1) {
        disk = rm.chain_rm_front<PmDiskObj>();
        if (disk == NULL) {
            break;
        }
        dsk_discovery_remove(disk);
        disk = NULL;  // free the disk obj when refcnt = 0
    }
    fds_assert(rm.chain_empty_list());
    LOGNORMAL << "End disk discovery...";
}

// dsk_dump_all
// ------------
//
void
PmDiskInventory::dsk_dump_all()
{
    DiskPrintIter iter;
    dsk_foreach(&iter);
}

// dsk_need_simulation
// -------------------
//
bool
PmDiskInventory::dsk_need_simulation()
{
    int hdd_count;

    /* Remove ssd disks */
    hdd_count = dsk_count - DISK_ALPHA_COUNT_SSD;
    if ((hdd_count <= dsk_qualify_cnt) && (hdd_count >= DISK_ALPHA_COUNT_HDD_MIN)) {
        return false;
    }
    LOGNORMAL << "Need to run in simulation";
    return true;
}

// disk_do_partition
// -----------------
//
void
PmDiskInventory::dsk_do_partition()
{
    DiskOp  op(DISK_OP_CHECK_PARTITION, dsk_partition);

    // For now, go through all HDD & check for parition.
    // dsk_foreach(dsk_partition, &op, &dsk_hdd, dsk_count);
}

// dsk_mount_all
// -------------
//
void
PmDiskInventory::dsk_mount_all()
{
}

// dsk_admit_all
// -------------
//
void
PmDiskInventory::dsk_admit_all()
{
}

// dsk_read_label
// --------------
//
bool
PmDiskInventory::dsk_read_label(DiskLabelMgr *mgr, bool creat)
{
    DiskLabelOp op(DISK_LABEL_READ, mgr);

    dsk_foreach(&op);
    return mgr->dsk_reconcile_label(this, creat);
}

// -----------------------------------------------------------------------------------
// Disk Platform Module
// -----------------------------------------------------------------------------------
DiskPlatModule gl_DiskPlatMod("Disk Module");

DiskPlatModule::DiskPlatModule(char const *const name) : Module(name) {}
DiskPlatModule::~DiskPlatModule() {}

// dsk_plat_singleton
// ------------------
//
DiskPlatModule *
DiskPlatModule::dsk_plat_singleton()
{
    return &gl_DiskPlatMod;
}

// mod_init
// --------
//
int
DiskPlatModule::mod_init(SysParams const *const param)
{
    dsk_devices = new PmDiskInventory();
    dsk_sim     = NULL;
    dsk_inuse   = NULL;
    dsk_ctrl    = udev_new();
    dsk_label   = new DiskLabelMgr();
    dsk_enum    = udev_enumerate_new(dsk_ctrl);
    fds_assert((dsk_ctrl != NULL) && (dsk_enum != NULL));

    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
DiskPlatModule::mod_startup()
{
    Module::mod_startup();
    udev_enumerate_add_match_subsystem(dsk_enum, "block");
    dsk_rescan();
    dsk_discover_mount_pts();

    if ((dsk_devices->dsk_read_label(dsk_label, false) == true) ||
        (dsk_devices->dsk_need_simulation() == false)) {
        /* Contain valid disk label or real HW inventory */
        dsk_inuse = dsk_devices;
    } else {
        const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
        dsk_sim   = new FileDiskInventory(dir->dir_dev().c_str());
        dsk_inuse = dsk_sim;
    }
    dsk_inuse->dsk_admit_all();
    dsk_inuse->dsk_mount_all();

    dsk_devices->dsk_dump_all();
    if (dsk_sim != NULL) {
        dsk_sim->dsk_dump_all();
    }
    dsk_inuse->dsk_read_label(dsk_label, true);
}

// dsk_discover_mount_pts
// ----------------------
//
void
DiskPlatModule::dsk_discover_mount_pts()
{
    FILE          *mnt;
    struct mntent *ent;

    mnt = setmntent("/proc/mounts", "r");
    if (mnt == NULL) {
        perror("setmnent");
        fds_panic("Can't open /proc/mounts");
    }
    while ((ent = getmntent(mnt)) != NULL) {
        if (strstr(ent->mnt_fsname, "/dev/sd") == NULL) {
            if (strstr(ent->mnt_fsname, "/dev/xvd") != NULL) {
                goto ok;
            }
            continue;
        }
 ok:
        PmDiskObj::pointer dsk = dsk_devices->dsk_get_info(ent->mnt_fsname);
        if (dsk == NULL) {
            LOGNORMAL << "Can't find maching dev " << ent->mnt_fsname;
        } else {
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
void
DiskPlatModule::mod_enable_service()
{
    Module::mod_enable_service();
}

// mod_shutdown
// ------------
//
void
DiskPlatModule::mod_shutdown()
{
    if (dsk_sim != NULL) {
        delete dsk_sim;
    }
    delete dsk_label;
    dsk_devices = NULL;
    dsk_inuse   = NULL;
    udev_enumerate_unref(dsk_enum);
    udev_unref(dsk_ctrl);
}

// dsk_rescan
// ----------
//
void
DiskPlatModule::dsk_rescan()
{
    bool                    blk;
    const char             *path;
    std::string             blk_path;
    std::string             dev_path;
    PmDiskObj::pointer      disk, slice;
    struct udev_list_entry *devices, *ptr;

    udev_enumerate_scan_devices(dsk_enum);
    devices = udev_enumerate_get_list_entry(dsk_enum);

    dsk_devices->dsk_discovery_begin();
    udev_list_entry_foreach(ptr, devices) {
        path = udev_list_entry_get_name(ptr);
        if (PmDiskObj::dsk_blk_dev_path(path, &blk_path, &dev_path) == false) {
            //
            // Only process block/sd devices.
            continue;
        }
        disk = dsk_devices->dsk_get_info(blk_path);
        if (disk == NULL) {
            slice = NULL;
        } else {
            slice = dsk_devices->dsk_get_info(blk_path, dev_path);
        }
        if (slice == NULL) {
            struct udev_device *dev;
            Resource::pointer   rs = dsk_devices->rs_alloc_new(ResourceUUID());

            slice = PmDiskObj::dsk_cast_ptr(rs);
            dev   = udev_device_new_from_syspath(dsk_ctrl, path);
            slice->dsk_update_device(dev, disk, blk_path, dev_path);
            dsk_devices->dsk_discovery_add(slice, disk);
        } else {
            slice->dsk_update_device(NULL, disk, blk_path, dev_path);
            dsk_devices->dsk_discovery_update(slice);
        }
    }
    dsk_devices->dsk_discovery_done();
}

// dsk_commit_label
// ----------------
//
void
DiskPlatModule::dsk_commit_label()
{
}

// -------------------------------------------------------------------------------------
// Disk simulation with files
// -------------------------------------------------------------------------------------
FileDiskObj::FileDiskObj(const char *dir) : PmDiskObj(), dsk_dir(dir)
{
    char path[FDS_MAX_FILE_NAME];

    FdsRootDir::fds_mkdir(dir);
    strncpy(rs_name, dir, sizeof(rs_name));

    snprintf(path, FDS_MAX_FILE_NAME, "%s/sblock", dir);
    dsk_fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
}

FileDiskObj::~FileDiskObj()
{
    close(dsk_fd);
}

// dsk_read
// --------
//
int
FileDiskObj::dsk_read(void *buf, fds_uint32_t sector, int sect_cnt)
{
    fds_assert(buf != NULL);
    return pread(dsk_fd, buf, sect_cnt * DL_SECTOR_SZ, 0);
}

// dsk_write
// ---------
//
int
FileDiskObj::dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt)
{
    int ret;

    ret = pwrite(dsk_fd, buf, sect_cnt * DL_SECTOR_SZ, 0);
    LOGNORMAL << rs_name << ", fd" << dsk_fd << ", ret " << ret
        << " : write at sector " << sector << ", cnt " << sect_cnt;
    return ret;
}

FileDiskInventory::FileDiskInventory(const char *dir) : PmDiskInventory()
{
    dsk_devdir = dir;
    FdsRootDir::fds_mkdir(dir);
}

FileDiskInventory::~FileDiskInventory()
{
    dsk_prev_inv.chain_transfer(&dsk_files);
    dsk_discovery_done();
}

// dsk_mount_all
// -------------
//
void
FileDiskInventory::dsk_mount_all()
{
}

// dsk_file_create
// ---------------
//
void
FileDiskInventory::dsk_file_create(const char *type, int count, ChainList *list)
{
    int          i, len, end;
    char        *cur, dir[FDS_MAX_FILE_NAME];
    FileDiskObj *file;

    len = snprintf(dir, sizeof(dir), "%s%s", dsk_devdir, type);
    cur = dir + len;
    len = sizeof(dir) - len;

    for (i = 0; i < count; i++) {
        end = snprintf(cur, len, "%d", i);
        LOGNORMAL << "Making " << dir << std::endl;
        file = new FileDiskObj(dir);
        file->dsk_read_uuid();

        rs_mtx.lock();
        DiskInventory::dsk_add_to_inventory_mtx(file, list);
        rs_mtx.unlock();
    }
}

// dsk_admit_all
// -------------
//
void
FileDiskInventory::dsk_admit_all()
{
    dsk_file_create("hdd-", DISK_ALPHA_COUNT_HDD, &dsk_files);
    dsk_file_create("ssd-", DISK_ALPHA_COUNT_SSD, &dsk_files);
}

// dsk_read_label
// --------------
//
bool
FileDiskInventory::dsk_read_label(DiskLabelMgr *mgr, bool creat)
{
    DiskLabelOp op(DISK_LABEL_READ, mgr);

    dsk_foreach(&op, &dsk_files, dsk_count);
    return mgr->dsk_reconcile_label(this, creat);
}

// dsk_do_partition
// ----------------
//
void
FileDiskInventory::dsk_do_partition()
{
}

// dsk_dump_all
// ------------
//
void
FileDiskInventory::dsk_dump_all()
{
    DiskPrintIter iter;
    dsk_foreach(&iter, &dsk_files, dsk_count);
}

}  // namespace fds
