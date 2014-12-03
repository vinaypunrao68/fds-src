/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_

#include <vector>

#include <fds_resource.h>

namespace fds {

class DiskObjIter;
class DiskInventory;

/**
 * Generic disk obj shared by PM and other daemons such as OM.
 */
class DiskObj : public Resource
{
  public:
    typedef boost::intrusive_ptr<DiskObj> pointer;
    typedef boost::intrusive_ptr<const DiskObj> const_ptr;

  protected:
    friend class DiskInventory;

    fds_uint64_t              dsk_cap_gb;
    dev_t                     dsk_my_devno;
    ChainLink                 dsk_type_link;        /**< link to hdd/ssd list...      */

  public:
    DiskObj();
    virtual ~DiskObj();

    static inline DiskObj::pointer dsk_cast_ptr(Resource::pointer ptr) {
        return static_cast<DiskObj *>(get_pointer(ptr));
    }
};

class DiskObjIter
{
  public:
    DiskObjIter();
    virtual ~DiskObjIter();

    /*
     * Return true to continue the iteration loop; false to quit.
     */
    virtual bool dsk_iter_fn(DiskObj::pointer curr) = 0;
    virtual bool dsk_iter_fn(DiskObj::pointer curr, DiskObjIter *cookie);
};

typedef std::vector<DiskObj::pointer> DiskObjArray;

class DiskInventory : public RsContainer
{
  protected:
    int                      dsk_count;
    ChainList                dsk_hdd;
    ChainList                dsk_ssd;

    Resource *rs_new(const ResourceUUID &uuid);
    int dsk_array_snapshot(ChainList *list, DiskObjArray *arr);

  public:
    DiskInventory();
    virtual ~DiskInventory();

    virtual void dsk_foreach(DiskObjIter *iter, ChainList *list, int count);
    virtual void dsk_foreach(DiskObjIter *iter, DiskObjIter *arg, ChainList *list, int);
    inline  void dsk_foreach(DiskObjIter *iter) {
        dsk_foreach(iter, &dsk_hdd, dsk_count);
    }
    virtual void dsk_add_to_inventory_mtx(DiskObj::pointer disk, ChainList *list);
    virtual void dsk_remove_out_inventory_mtx(DiskObj::pointer disk);
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_
