/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_

#include <vector>

#include "disk_object.h"

namespace fds
{
    typedef std::vector<DiskObject::pointer> DiskObjArray;

    class DiskInventory : public RsContainer
    {
        protected:
            int          dsk_count;
            ChainList    dsk_hdd;
            ChainList    dsk_ssd;

            Resource *rs_new(const ResourceUUID &uuid);
            int dsk_array_snapshot(ChainList *list, DiskObjArray *arr);

        public:
            DiskInventory();
            virtual ~DiskInventory();

            virtual void dsk_foreach(DiskObjIter *iter, ChainList *list, int count);
            virtual void dsk_foreach(DiskObjIter *iter, DiskObjIter *arg, ChainList *list, int);

            inline void dsk_foreach(DiskObjIter *iter)
            {
                dsk_foreach(iter, &dsk_hdd, dsk_count);
            }

            virtual void dsk_add_to_inventory_mtx(DiskObject::pointer disk, ChainList *list);
            virtual void dsk_remove_out_inventory_mtx(DiskObject::pointer disk);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_INVENTORY_H_
