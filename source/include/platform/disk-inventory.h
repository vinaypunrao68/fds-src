/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_DISK_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_DISK_INVENTORY_H_

#include <fds_resource.h>

namespace fds {

class DiskObjIter;

/**
 * Generic disk obj shared by PM and other daemons such as OM.
 */
class DiskObj : public Resource
{
  public:
    typedef boost::intrusive_ptr<DiskObj> pointer;
    typedef boost::intrusive_ptr<const DiskObj> const_ptr;

  protected:
    fds_uint64_t              dsk_cap_gb;
    dev_t                     dsk_my_devno;

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
    DiskObjIter() {}
    virtual ~DiskObjIter() {}

    /*
     * Return true to continue the iteration loop; false to quit.
     */
    virtual bool dsk_iter_fn(DiskObj::pointer curr) = 0;
};

typedef std::vector<DiskObj::pointer> DiskObjArray;

class DiskInventory : public RsContainer
{
  protected:
    int                      dsk_count;
    ChainList                dsk_hdd;
    ChainList                dsk_ssd;

    Resource *rs_new(const ResourceUUID &uuid);

  public:
    DiskInventory();
    virtual ~DiskInventory();

    virtual void dsk_foreach(DiskObjIter *iter);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_DISK_INVENTORY_H_
