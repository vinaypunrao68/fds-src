/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_DISK_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_H_

#include <libudev.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <iostream>  // NOLINT
#include <cpplist.h>
#include <fds_module.h>
#include <fds_resource.h>
#include <shared/fds_types.h>

namespace fds {

class DiskObj;
class DiskInventory;
class DiskPlatModule;

/**
 * Common info about a disk shared among the main obj and its partitions (e.g. sda,
 * sda1, sda2...).  The main obj is responsible to free it after freeing all child objs
 * representing partitions.
 */
class DiskCommon
{
  protected:
    friend class DiskObj;
    friend class DiskInventory;

    std::string               dsk_blk_path;
    fds_disk_type_t           dsk_type;
    fds_tier_type_e           dsk_tier;

  public:
    explicit DiskCommon(const std::string &blk);
    virtual ~DiskCommon();
    inline const std::string &dsk_get_blk_path() { return dsk_blk_path; }
};

class DiskObj : public Resource
{
  public:
    typedef boost::intrusive_ptr<DiskObj> pointer;
    typedef boost::intrusive_ptr<const DiskObj> const_ptr;

  protected:
    friend class DiskInventory;
    friend std::ostream &operator<< (std::ostream &, fds::DiskObj::pointer obj);

    ChainLink                 dsk_part_link;      /**< link to dsk_partition list. */
    ChainLink                 dsk_disc_link;      /**< link to discovery list.     */
    ChainLink                 dsk_type_link;      /**< link to hdd/ssd list.       */
    ChainList                 dsk_part_head;      /**< sda -> { sda1, sda2... }    */

    const char               *dsk_raw_path;
    int                       dsk_raw_plen;
    int                       dsk_part_idx;
    int                       dsk_part_cnt;
    fds_uint64_t              dsk_cap_gb;

    dev_t                     dsk_my_devno;
    DiskObj::pointer          dsk_parent;
    struct udev_device       *dsk_my_dev;
    DiskCommon               *dsk_common;

  public:
    DiskObj();
    virtual ~DiskObj();

    /**
     * Return HW device path from raw path.
     * @return: example
     *    blk : /devices/pci0000:00/0000:00:0d.0/ata3/host2/target2:0:0/2:0:0:0/block/
     *    dev : /dev/sda
     */
    static bool dsk_blk_dev_path(const char *raw, std::string *blk, std::string *dev);
    static inline DiskObj::pointer dsk_cast_ptr(Resource::pointer ptr) {
        return static_cast<DiskObj *>(get_pointer(ptr));
    }
    /**
     * Return the slice device matching with the dev_path (e.g. /dev/sda, /dev/sda1...)
     */
    virtual DiskObj::pointer dsk_get_info(const std::string &dev_path);

    /**
     * Update the disk obj during the discovery process.
     * @param dev (i) - NULL if the obj already has the udev device.
     * @param ref (i) - sibling or parent object.  May need to fixup the chain.
     * @param b_path (i) - common path to the block device (e.g /devices/pci...)
     * @param d_path (i) - the device path from dev (e.g. /dev/sda1...)
     */
    virtual void dsk_update_device(struct udev_device *dev,
                                   DiskObj::pointer    ref,
                                   const std::string  &b_path,
                                   const std::string  &d_path);

  private:
    void dsk_read_uuid();
    void dsk_fixup_family_links(DiskObj::pointer ref);
};

typedef std::unordered_map<std::string, DiskObj::pointer> DiskDevMap;

class DiskInventory : public RsContainer
{
  protected:
    DiskDevMap               dsk_dev_map;
    ChainList                dsk_discovery;
    ChainList                dsk_prev_inv;       /**< previous inventory list.     */
    ChainList                dsk_curr_inv;       /**< disks that were enumerated.  */
    ChainList                dsk_hdd;
    ChainList                dsk_ssd;

    Resource *rs_new(const ResourceUUID &uuid);

  public:
    typedef boost::intrusive_ptr<DiskInventory> pointer;
    typedef boost::intrusive_ptr<const DiskInventory> const_ptr;

    DiskInventory();
    virtual ~DiskInventory();

    virtual DiskObj::pointer dsk_get_info(const ResourceUUID &uuid) {
        return DiskObj::dsk_cast_ptr(rs_get_resource(uuid));
    }
    virtual DiskObj::pointer dsk_get_info(dev_t dev_node);
    virtual DiskObj::pointer dsk_get_info(dev_t dev_node, int partition);
    virtual DiskObj::pointer dsk_get_info(const std::string &b);
    virtual DiskObj::pointer dsk_get_info(const std::string &b, const std::string &d);

    virtual void dsk_dump_all();
    virtual void dsk_discovery_begin();
    virtual void dsk_discovery_add(DiskObj::pointer disk, DiskObj::pointer ref);
    virtual void dsk_discovery_update(DiskObj::pointer disk);
    virtual void dsk_discovery_remove(DiskObj::pointer disk);
    virtual void dsk_discovery_done();
};

/**
 * Main module controlling disk/storage HW inventory.
 */
class DiskPlatModule : public Module
{
  protected:
    DiskInventory            dsk_devices;
    struct udev             *dsk_ctrl;
    struct udev_enumerate   *dsk_enum;

  public:
    explicit DiskPlatModule(char const *const name);
    virtual ~DiskPlatModule();

    static DiskPlatModule *dsk_plat_singleton();
    static inline DiskInventory::pointer dsk_get_hdd_inv() {
        return &dsk_plat_singleton()->dsk_devices;
    }
    static inline DiskInventory::pointer dsk_get_ssd_inv() {
        return &dsk_plat_singleton()->dsk_devices;
    }

    void dsk_rescan();

    // Module methods.
    ///
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

extern DiskPlatModule gl_DiskPlatMod;

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_H_
