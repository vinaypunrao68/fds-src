/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <fds_uuid.h>
#include <disk.h>

namespace fds {

DiskCommon::DiskCommon(const std::string &blk) : dsk_blk_path(blk) {}
DiskCommon::~DiskCommon() {}

DiskObj::~DiskObj()
{
    if (dsk_my_dev != NULL) {
        udev_device_unref(dsk_my_dev);
        dsk_my_dev = NULL;
    }
    if (get_pointer(dsk_parent) == this) {
        fds_verify(dsk_common != NULL);
        delete dsk_common;

    } else if (dsk_parent != NULL) {
        dsk_parent->rs_mutex()->lock();
        dsk_part_link.chain_rm_init();
        fds_verify(dsk_common == dsk_parent->dsk_common);
        dsk_parent->rs_mutex()->unlock();

        dsk_common = NULL;
        dsk_parent = NULL;  // free parent obj when refcnt = 0.
    } else {
        fds_verify(dsk_common == NULL);
    }
    fds_verify(dsk_part_link.chain_empty());
    fds_verify(dsk_disc_link.chain_empty());
    fds_verify(dsk_type_link.chain_empty());
    fds_verify(dsk_part_head.chain_empty_list());
}

DiskObj::DiskObj()
    : Resource(ResourceUUID(0)), dsk_part_link(this), dsk_disc_link(this),
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
DiskObj::dsk_update_device(struct udev_device *dev,
                           DiskObj::pointer    ref,
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
DiskObj::pointer
DiskObj::dsk_get_info(const std::string &dev_path)
{
    ChainList        *slices;
    ChainIter         iter;
    DiskObj::pointer  found;

    found = NULL;
    if (dsk_parent != NULL) {
        fds_verify(dsk_parent->dsk_parent == dsk_parent);
        fds_assert(dsk_parent->dsk_common == dsk_common);

        dsk_parent->rs_mutex()->lock();
        slices = &dsk_parent->dsk_part_head;
        for (slices->chain_iter_init(&iter);
             !slices->chain_iter_term(iter); slices->chain_iter_next(&iter)) {
            found = slices->chain_iter_current<DiskObj>(iter);
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
DiskObj::dsk_read_uuid()
{
    rs_uuid.uuid_set_val(fds_get_uuid64(get_uuid()));
}

// dsk_fixup_family_links
// ----------------------
//
void
DiskObj::dsk_fixup_family_links(DiskObj::pointer ref)
{
    ChainIter         iter;
    ChainList        *slices, tmp;
    DiskObj::pointer  parent, disk;

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
            disk = slices->chain_iter_current<DiskObj>(iter);
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
        parent->dsk_part_head.chain_add_back(&dsk_part_link);
        parent->rs_mutex()->unlock();
    }
}

std::ostream &operator<< (std::ostream &os, DiskObj::pointer obj)
{
    ChainIter   i;
    ChainList  *l;

    if (obj->dsk_parent == obj) {
        os << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec << "]\n";
        os << obj->dsk_common->dsk_get_blk_path() << std::endl;
        os << obj->dsk_raw_path << std::endl;
    } else {
        os << "  " << obj->rs_name << " [uuid " << std::hex
           << obj->rs_uuid.uuid_get_val() << std::dec << "]\n";
        os << "  " << obj->dsk_raw_path << "\n";
    }
    // Not thread safe traversal.
    l = &obj->dsk_part_head;
    for (l->chain_iter_init(&i); !l->chain_iter_term(i); l->chain_iter_next(&i)) {
        DiskObj::pointer disk = l->chain_iter_current<DiskObj>(i);
        os << disk;
    }
    os << std::endl;
    return os;
}

// dsk_blk_dev_path
// ----------------
//
bool
DiskObj::dsk_blk_dev_path(const char *raw, std::string *blk, std::string *dev)
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

DiskInventory::~DiskInventory() {}
DiskInventory::DiskInventory() : RsContainer()
{
}

// rs_new
// ------
//
Resource *
DiskInventory::rs_new(const ResourceUUID &uuid)
{
    return new DiskObj();
}

// dsk_get_info
// ------------
//
DiskObj::pointer
DiskInventory::dsk_get_info(dev_t dev_node)
{
    return NULL;
}

DiskObj::pointer
DiskInventory::dsk_get_info(dev_t dev_node, int partition)
{
    return NULL;
}

DiskObj::pointer
DiskInventory::dsk_get_info(const std::string &blk_path)
{
    DiskObj::pointer disk;

    rs_mtx.lock();
    disk = dsk_dev_map[blk_path];
    rs_mtx.unlock();

    return disk;
}

DiskObj::pointer
DiskInventory::dsk_get_info(const std::string &blk_path, const std::string &dev_path)
{
    DiskObj::pointer disk;

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
DiskInventory::dsk_discovery_begin()
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
DiskInventory::dsk_discovery_add(DiskObj::pointer disk, DiskObj::pointer ref)
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
DiskInventory::dsk_discovery_update(DiskObj::pointer disk)
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
DiskInventory::dsk_discovery_remove(DiskObj::pointer disk)
{
    fds_verify(disk->dsk_common != NULL);
    rs_mtx.lock();
    fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);

    // Unlink the chain and unmap every indices.
    disk->dsk_disc_link.chain_rm_init();
    disk->dsk_type_link.chain_rm_init();

    dsk_dev_map.erase(disk->dsk_common->dsk_blk_path);
    rs_unregister_mtx(disk);
    rs_mtx.unlock();

    // Break free from the inventory.
    rs_free_resource(disk);
}

// dsk_discovery_done
// ------------------
//
void
DiskInventory::dsk_discovery_done()
{
    DiskObj::pointer disk;

    rs_mtx.lock();
    dsk_curr_inv.chain_transfer(&dsk_discovery);
    fds_assert(dsk_discovery.chain_empty_list());

    // Anything left over in the prev inventory list are phantom devices.
    while (1) {
        disk = dsk_prev_inv.chain_rm_front<DiskObj>();
        if (disk == NULL) {
            break;
        }
        dsk_discovery_remove(disk);
        disk = NULL;  // free the disk obj when refcnt = 0
    }
    rs_mtx.unlock();
}

// dsk_dump_all
// ------------
//
void
DiskInventory::dsk_dump_all()
{
    ChainList       *l;
    ChainIter        i;
    DiskObj::pointer disk;

    l = &dsk_hdd;
    for (l->chain_iter_init(&i); !l->chain_iter_term(i); l->chain_iter_next(&i)) {
        disk = l->chain_iter_current<DiskObj>(i);
        std::cout << disk;
    }
}

// -----------------------------------------------------------------------------------
// Disk Platform Module
// -----------------------------------------------------------------------------------
DiskPlatModule gl_DiskPlatMod("Disk Module");

DiskPlatModule::DiskPlatModule(char const *const name) : Module(name)
{
    dsk_ctrl = udev_new();
    dsk_enum = udev_enumerate_new(dsk_ctrl);
    fds_assert((dsk_ctrl != NULL) && (dsk_enum != NULL));
}

DiskPlatModule::~DiskPlatModule()
{
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
}

// mod_shutdown
// ------------
//
void
DiskPlatModule::mod_shutdown()
{
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
    DiskObj::pointer        disk, slice;
    struct udev_list_entry *devices, *ptr;

    udev_enumerate_scan_devices(dsk_enum);
    devices = udev_enumerate_get_list_entry(dsk_enum);

    dsk_devices.dsk_discovery_begin();
    udev_list_entry_foreach(ptr, devices) {
        path = udev_list_entry_get_name(ptr);
        if (DiskObj::dsk_blk_dev_path(path, &blk_path, &dev_path) == false) {
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

            slice = DiskObj::dsk_cast_ptr(rs);
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

}  // namespace fds
