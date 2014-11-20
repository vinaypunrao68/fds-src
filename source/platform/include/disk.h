/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_DISK_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_H_

#include <libudev.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <sys/poll.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>  // NOLINT
#include <cpplist.h>
#include <fds_module.h>
#include <shared/fds-constants.h>
#include <platform/disk-inventory.h>

namespace fds {

class PmDiskObj;
class PmDiskInventory;
class DiskLabel;
class DiskLabelMgr;
class DiskPlatModule;
class DiskPartitionMgr;
class FileDiskInventory;

/**
 * Common info about a disk shared among the main obj and its partitions (e.g. sda,
 * sda1, sda2...).  The main obj is responsible to free it after freeing all child objs
 * representing partitions.
 */
class DiskCommon
{
  protected:
    friend class PmDiskObj;
    friend class PmDiskInventory;

    std::string               dsk_blk_path;
    fds_disk_type_t           dsk_type;
    fds_tier_type_e           dsk_tier;

  public:
    explicit DiskCommon(const std::string &blk);
    virtual ~DiskCommon();

    inline const std::string &dsk_get_blk_path() { return dsk_blk_path; }
};

class PmDiskObj : public DiskObj
{
  public:
    typedef boost::intrusive_ptr<PmDiskObj> pointer;
    typedef boost::intrusive_ptr<const PmDiskObj> const_ptr;

  protected:
    friend class PmDiskInventory;
    friend std::ostream &operator<< (std::ostream &, fds::PmDiskObj::pointer obj);

    ChainLink                 dsk_part_link;      /**< link to dsk_partition list. */
    ChainLink                 dsk_disc_link;      /**< link to discovery list.     */
    ChainList                 dsk_part_head;      /**< sda -> { sda1, sda2... }    */
                                                  /**< /dev/sda is in rs_name.     */
    std::string               dsk_mount_pt;       /**< PL's mount point.           */
    const char               *dsk_raw_path;
    int                       dsk_raw_plen;
    int                       dsk_part_idx;
    int                       dsk_part_cnt;
    fds_uint64_t              dsk_cap_gb;

    dev_t                     dsk_my_devno;
    PmDiskObj::pointer        dsk_parent;
    struct udev_device       *dsk_my_dev;
    DiskCommon               *dsk_common;
    DiskLabel                *dsk_label;

  public:
    PmDiskObj();
    virtual ~PmDiskObj();

    /**
     * Return HW device path from raw path.
     * @return: true if the raw path is block SD device; false otherwise.
     * @param blk : /devices/pci00:00/0000:00:0d.0/ata3/host2/target2:0:0/2:0:0:0/block/
     * @param dev : /dev/sda
     */
    static bool dsk_blk_dev_path(const char *raw, std::string &blk, std::string &dev);
    static inline PmDiskObj::pointer dsk_cast_ptr(DiskObj::pointer ptr) {
        return static_cast<PmDiskObj *>(get_pointer(ptr));
    }
    static inline PmDiskObj::pointer dsk_cast_ptr(Resource::pointer ptr) {
        return static_cast<PmDiskObj *>(get_pointer(ptr));
    }
    inline PmDiskObj::pointer dsk_get_parent() { return dsk_parent; }
    inline DiskLabel *dsk_xfer_label() {
        DiskLabel *ret = dsk_label; dsk_label = NULL; return ret;
    }
    inline fds_uint64_t dsk_capacity_gb() { return dsk_cap_gb; }

    /**
     * Setup the mount point where the device is mounted as the result of the discovery.
     */
    void dsk_set_mount_point(const char *mnt);
    const std::string &dsk_get_mount_point();

    /**
     * Read in disk label.  Generate uuid for the disk if we don't have valid label.
     */
    void dsk_read_uuid();

    /**
     * Raw sector read/write to support super block update.  Only done in parent disk.
     */
    virtual ssize_t dsk_read(void *buf, fds_uint32_t sector, int sect_cnt);
    virtual ssize_t dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt);

    /**
     * Return the slice device matching with the dev_path (e.g. /dev/sda, /dev/sda1...)
     */
    virtual PmDiskObj::pointer dsk_get_info(const std::string &dev_path);

    /**
     * Update the disk obj during the discovery process.
     * @param dev (i) - NULL if the obj already has the udev device.
     * @param ref (i) - sibling or parent object.  May need to fixup the chain.
     * @param b_path (i) - common path to the block device (e.g /devices/pci...)
     * @param d_path (i) - the device path from dev (e.g. /dev/sda1...)
     */
    virtual void dsk_update_device(struct udev_device *dev,
                                   PmDiskObj::pointer  ref,
                                   const std::string  &b_path,
                                   const std::string  &d_path);

    /**
     * Iterate the parent object for all sub devices attached to it.
     */
    virtual void dsk_dev_foreach(DiskObjIter *iter, bool parent_only = false);

  private:
    void dsk_fixup_family_links(PmDiskObj::pointer ref);
};

typedef std::unordered_map<std::string, PmDiskObj::pointer> DiskDevMap;

class PmDiskInventory : public DiskInventory
{
  protected:
    DiskDevMap               dsk_dev_map;
    uint32_t                 dsk_qualify_cnt;    /**< disks that can run FDS SW.   */
    ChainList                dsk_discovery;
    ChainList                dsk_prev_inv;       /**< previous inventory list.     */
    ChainList                dsk_curr_inv;       /**< disks that were enumerated.  */
    DiskPartitionMgr        *dsk_partition;

    Resource *rs_new(const ResourceUUID &uuid);

  public:
    typedef boost::intrusive_ptr<PmDiskInventory> pointer;
    typedef boost::intrusive_ptr<const PmDiskInventory> const_ptr;

    PmDiskInventory();
    virtual ~PmDiskInventory();

    /**
     * Return true if the discovered inventory needs disk simulation.
     */
    bool dsk_need_simulation();

    virtual PmDiskObj::pointer dsk_get_info(const ResourceUUID &uuid) {
        return PmDiskObj::dsk_cast_ptr(rs_get_resource(uuid));
    }
    virtual PmDiskObj::pointer dsk_get_info(const char *name) {
        return PmDiskObj::dsk_cast_ptr(rs_get_resource(name));
    }
    virtual PmDiskObj::pointer dsk_get_info(dev_t dev_node);
    virtual PmDiskObj::pointer dsk_get_info(dev_t dev_node, int partition);
    virtual PmDiskObj::pointer dsk_get_info(const std::string &b);
    virtual PmDiskObj::pointer dsk_get_info(const std::string &b, const std::string &d);

    virtual void dsk_dump_all();
    virtual void dsk_discovery_begin();
    virtual void dsk_discovery_add(PmDiskObj::pointer disk, PmDiskObj::pointer ref);
    virtual void dsk_discovery_update(PmDiskObj::pointer disk);
    virtual void dsk_discovery_remove(PmDiskObj::pointer disk);
    virtual void dsk_discovery_done();

    virtual void dsk_do_partition();
    virtual void dsk_admit_all();
    virtual void dsk_mount_all();
    virtual bool disk_read_label(DiskLabelMgr *mgr, bool creat);
};

/**
 * Main module controlling disk/storage HW inventory.
 */
class DiskPlatModule : public Module
{
  protected:
    PmDiskInventory::pointer  dsk_devices;
    PmDiskInventory::pointer  dsk_inuse;
    struct udev              *dsk_ctrl;
    struct udev_enumerate    *dsk_enum;
    struct udev_monitor      *dsk_mon;
    FileDiskInventory        *dsk_sim;
    DiskLabelMgr             *label_manager;
    struct pollfd            pollfds[1];

    void dsk_discover_mount_pts();

  public:
    explicit DiskPlatModule(char const *const name);
    virtual ~DiskPlatModule();

    static DiskPlatModule *dsk_plat_singleton();
    static inline PmDiskInventory::pointer dsk_get_hdd_inv() {
        return dsk_plat_singleton()->dsk_devices;
    }
    static inline PmDiskInventory::pointer dsk_get_ssd_inv() {
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

extern DiskPlatModule gl_DiskPlatMod;

// -------------------------------------------------------------------------------------
// Development simulation
// -------------------------------------------------------------------------------------

/**
 * Use file to simulate a disk.
 */
class FileDiskInventory : public PmDiskInventory
{
  protected:
    ChainList                dsk_files;
    const char              *dsk_devdir;

  public:
    explicit FileDiskInventory(const char *dir);
    ~FileDiskInventory();

    virtual void dsk_dump_all();
    virtual void dsk_do_partition();
    virtual void dsk_admit_all();
    virtual void dsk_mount_all();
    virtual bool disk_read_label(DiskLabelMgr *mgr, bool creat);

  private:
    void dsk_file_create(const char *type, int count, ChainList *list);
};

class FileDiskObj : public PmDiskObj
{
  protected:
    friend class FileDiskInventory;

    int                      dsk_fd;
    const char              *dsk_dir;

  public:
    explicit FileDiskObj(const char *dir);
    ~FileDiskObj();

    virtual ssize_t dsk_read(void *buf, fds_uint32_t sector, int sect_cnt);
    virtual ssize_t dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt);
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_H_
