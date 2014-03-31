/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <fds_uuid.h>
#include <fds_process.h>
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
    fds_verify(dsk_part_link.chain_empty());
    fds_verify(dsk_disc_link.chain_empty());
    fds_verify(dsk_type_link.chain_empty());
    fds_verify(dsk_part_head.chain_empty_list());

    std::cout << "free " << rs_name << std::endl;
}

PmDiskObj::PmDiskObj()
    : dsk_part_link(this), dsk_disc_link(this),
      dsk_type_link(this), dsk_raw_path(NULL), dsk_parent(NULL),
      dsk_my_dev(NULL), dsk_common(NULL)
{
    dsk_cap_gb   = 0;
    dsk_part_idx = 0;
    dsk_part_cnt = 0;
    dsk_my_devno = 0;
    dsk_raw_plen = 0;
}

// dsk_update_device
// -----------------
//
void
PmDiskObj::dsk_update_device(struct udev_device *dev,
                             PmDiskObj::pointer    ref,
                             const std::string  &blk_path,
                             const std::string  &dev_path)
{
    if (dev == NULL) {
        dev = dsk_my_dev;
    } else {
        fds_assert(dsk_my_dev == NULL);
        fds_assert(dsk_parent == NULL);
        dsk_my_dev = dev;
        dsk_read_uuid();
    }
    dsk_cap_gb   = strtoul(udev_device_get_sysattr_value(dev, "size"), NULL, 10);
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
        for (slices->chain_iter_init(&iter);
             !slices->chain_iter_term(iter); slices->chain_iter_next(&iter)) {
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
    rs_uuid.uuid_set_val(fds_get_uuid64(get_uuid()));
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
    for (l->chain_iter_init(&i); !l->chain_iter_term(i); l->chain_iter_next(&i)) {
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
        for (slices->chain_iter_init(&iter);
             !slices->chain_iter_term(iter); slices->chain_iter_next(&iter)) {
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
        std::cout << PmDiskObj::dsk_cast_ptr(disk);
        return true;
    }
};

std::ostream &operator<< (std::ostream &os, PmDiskObj::pointer obj)
{
    ChainIter   i;
    ChainList  *l;

    if (obj->dsk_parent == obj) {
        os << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec << "]\n";
        os << obj->dsk_common->dsk_get_blk_path() << std::endl;
        os << obj->dsk_raw_path << std::endl;

        DiskPrintIter iter;
        obj->dsk_dev_foreach(&iter);
    } else {
        os << "  " << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec << "]\n";
        os << "  " << obj->dsk_raw_path << "\n";
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
        return false;
    }
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
PmDiskObj::dsk_read(void *buf, fds_uint32_t sector, int sect_cnt)
{
    return 0;
}

// dsk_write
// ---------
//
int
PmDiskObj::dsk_write(void *buf, fds_uint32_t sector, int sec_cnt)
{
    return 0;
}

// -------------------------------------------------------------------------------------
// PM Disk Inventory
// -------------------------------------------------------------------------------------
PmDiskInventory::PmDiskInventory() : DiskInventory()
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
    rs_mtx.lock();
    if (dsk_prev_inv.chain_empty_list() && dsk_discovery.chain_empty_list()) {
        dsk_prev_inv.chain_transfer(&dsk_curr_inv);
        fds_assert(dsk_curr_inv.chain_empty_list());
    }
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
        // TODO(Vy): move this code to shared/disk-inventory.cpp
        dsk_count++;
        dsk_hdd.chain_add_back(&disk->dsk_type_link);
    }
    // Add to the map indexed by uuid/dev path.
    rs_register_mtx(disk);
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
    disk->dsk_type_link.chain_rm_init();

    if (disk->dsk_parent == disk) {
        fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);
        dsk_dev_map.erase(disk->dsk_common->dsk_blk_path);
        disk->dsk_parent = NULL;  // dec the refcnt.
    }
    rs_unregister_mtx(disk);
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
    if (dsk_count != (DISK_ALPHA_COUNT_HDD + DISK_ALPHA_COUNT_SSD)) {
        return true;
    }
    return false;
}

// dsk_foreach
// -----------
//
void
PmDiskInventory::dsk_foreach(DiskObjIter *iter)
{
    int           cnt;
    ChainIter     i;
    ChainList    *l;
    DiskObjArray  disks(dsk_dev_map.size() << 1);

    cnt = 0;
    l = &dsk_hdd;
    rs_mtx.lock();
    for (l->chain_iter_init(&i); !l->chain_iter_term(i); l->chain_iter_next(&i)) {
        disks[cnt++] = l->chain_iter_current<PmDiskObj>(i);
    }
    rs_mtx.unlock();

    for (int j = 0; j < cnt; j++) {
        if (iter->dsk_iter_fn(disks[j]) == false) {
            break;
        }
    }
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

// -----------------------------------------------------------------------------------
// Disk Platform Module
// -----------------------------------------------------------------------------------
DiskPlatModule gl_DiskPlatMod("Disk Module");

DiskPlatModule::DiskPlatModule(char const *const name) : Module(name)
{
    dsk_sim   = NULL;
    dsk_inuse = NULL;
    dsk_ctrl  = udev_new();
    dsk_enum  = udev_enumerate_new(dsk_ctrl);
    fds_assert((dsk_ctrl != NULL) && (dsk_enum != NULL));
}

DiskPlatModule::~DiskPlatModule()
{
    mod_shutdown();
    udev_enumerate_unref(dsk_enum);
    udev_unref(dsk_ctrl);
}

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
    return 0;
}

// mod_startup
// -----------
//
void
DiskPlatModule::mod_startup()
{
    udev_enumerate_add_match_subsystem(dsk_enum, "block");
    dsk_rescan();
    dsk_devices.dsk_dump_all();

    if (dsk_devices.dsk_need_simulation() == true) {
        const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
        dsk_sim   = new FileDiskInventory(dir->dir_dev().c_str());
        dsk_inuse = dsk_sim;
    } else {
        dsk_inuse = &dsk_devices;
    }
    dsk_inuse->dsk_admit_all();
    dsk_inuse->dsk_mount_all();
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

    dsk_devices.dsk_discovery_begin();
    udev_list_entry_foreach(ptr, devices) {
        path = udev_list_entry_get_name(ptr);
        if (PmDiskObj::dsk_blk_dev_path(path, &blk_path, &dev_path) == false) {
            //
            // Only process block/sd devices.
            continue;
        }
        disk = dsk_devices.dsk_get_info(blk_path);
        if (disk == NULL) {
            slice = NULL;
        } else {
            slice = dsk_devices.dsk_get_info(blk_path, dev_path);
        }
        if (slice == NULL) {
            struct udev_device *dev;
            Resource::pointer   rs = dsk_devices.rs_alloc_new(ResourceUUID(0));

            slice = PmDiskObj::dsk_cast_ptr(rs);
            dev   = udev_device_new_from_syspath(dsk_ctrl, path);
            slice->dsk_update_device(dev, disk, blk_path, dev_path);
            dsk_devices.dsk_discovery_add(slice, disk);
        } else {
            slice->dsk_update_device(NULL, disk, blk_path, dev_path);
            dsk_devices.dsk_discovery_update(slice);
        }
    }
    dsk_devices.dsk_discovery_done();
}

// -------------------------------------------------------------------------------------
// Disk simulation with files
// -------------------------------------------------------------------------------------
FileDiskObj::FileDiskObj(const char *dir) : PmDiskObj(), dsk_dir(dir)
{
    FdsRootDir::fds_mkdir(dir);
    rs_uuid.uuid_set_val(fds_get_uuid64(dir));
    snprintf(rs_name, sizeof(rs_name), "%s/sblock", dir);

    dsk_fd = open(rs_name, O_DIRECT | O_CREAT, S_IRUSR | S_IXUSR);
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
    fds_assert(dsk_fd > 0);

    return read(dsk_fd, buf, sect_cnt * 512);
}

// dsk_write
// ---------
//
int
FileDiskObj::dsk_write(void *buf, fds_uint32_t sector, int sect_cnt)
{
    return 0;
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
        std::cout << "Making " << dir << std::endl;
        file = new FileDiskObj(dir);

        rs_mtx.lock();
        list->chain_add_back(&file->dsk_type_link);
        rs_register_mtx(file);
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

// dsk_do_partition
// ----------------
//
void
FileDiskInventory::dsk_do_partition()
{
}

}  // namespace fds
