/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "disk_obj_iter.h"
#include "disk_inventory.h"

namespace fds
{
    DiskInventory::DiskInventory() :    RsContainer(), dsk_count(0)
    {
    }

    DiskInventory::~DiskInventory()
    {
    }

    Resource *DiskInventory::rs_new(const ResourceUUID &uuid)
    {
        return new DiskObject();
    }

    // dsk_array_snapshot
    // ------------------
    //
    int DiskInventory::dsk_array_snapshot(ChainList *list, DiskObjArray *arr)
    {
        int          cnt;
        ChainIter    iter;

        cnt = 0;
        rs_mtx.lock();
        for (list->chain_iter_init(&iter); !list->chain_iter_term(iter);
             list->chain_iter_next(&iter))
        {
            (*arr)[cnt++] = list->chain_iter_current<DiskObject>(iter);
        }

        rs_mtx.unlock();
        return cnt;
    }

    // dsk_add_to_inventory_mtx
    // ------------------------
    // Add the disk to the inventory.  The caller must hold the mutex.
    //
    void DiskInventory::dsk_add_to_inventory_mtx(DiskObject::pointer disk, ChainList *list)
    {
        if (list != NULL)
        {
            dsk_count++;
            list->chain_add_back(&disk->dsk_type_link);
        }

        rs_register_mtx(disk);
    }

    // dsk_remove_out_inventory_mtx
    // ----------------------------
    // Remove the disk out of the inventory.  The caller must hold the mutex.
    //
    void DiskInventory::dsk_remove_out_inventory_mtx(DiskObject::pointer disk)
    {
        if (!disk->dsk_type_link.chain_empty())
        {
            dsk_count--;
            disk->dsk_type_link.chain_rm_init();
        }

        rs_unregister_mtx(disk);
    }

    // dsk_foreach
    // -----------
    //
    void DiskInventory::dsk_foreach(DiskObjIter *iter, ChainList *list, int count)
    {
        int             cnt;
        DiskObjArray    disks(count << 1);

        cnt = dsk_array_snapshot(list, &disks);

        for (int j = 0; j < cnt; j++)
        {
            if (iter->dsk_iter_fn(disks[j]) == false)
            {
                break;
            }
        }
    }

    void DiskInventory::dsk_foreach(DiskObjIter *iter, DiskObjIter *arg, ChainList   *list,
                                    int count)
    {
        int             cnt;
        DiskObjArray    disks(count << 1);

        cnt = dsk_array_snapshot(list, &disks);

        for (int j = 0; j < cnt; j++)
        {
            if (iter->dsk_iter_fn(disks[j], arg) == false)
            {
                break;
            }
        }
    }
}  // namespace fds
