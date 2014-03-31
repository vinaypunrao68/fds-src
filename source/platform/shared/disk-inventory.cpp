/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/disk-inventory.h>

namespace fds {

DiskObj::DiskObj() : Resource(ResourceUUID(0)) {}
DiskObj::~DiskObj() {}

DiskInventory::DiskInventory() : RsContainer(), dsk_count(0) {}
DiskInventory::~DiskInventory() {}

DiskObjIter::DiskObjIter() {}
DiskObjIter::~DiskObjIter() {}

bool
DiskObjIter::dsk_iter_fn(DiskObj::pointer curr, DiskObjIter *cookie)
{
    return dsk_iter_fn(curr);
}

Resource *
DiskInventory::rs_new(const ResourceUUID &uuid)
{
    return new DiskObj();
}

// dsk_array_snapshot
// ------------------
//
int
DiskInventory::dsk_array_snapshot(ChainList *list, DiskObjArray *arr)
{
    int       cnt;
    ChainIter iter;

    cnt = 0;
    rs_mtx.lock();
    for (list->chain_iter_init(&iter);
         !list->chain_iter_term(iter); list->chain_iter_next(&iter)) {
        (*arr)[cnt++] = list->chain_iter_current<DiskObj>(iter);
    }
    rs_mtx.unlock();
    return cnt;
}

// dsk_foreach
// -----------
//
void
DiskInventory::dsk_foreach(DiskObjIter *iter, ChainList *list, int count)
{
    int           cnt;
    DiskObjArray  disks(count << 1);

    cnt = dsk_array_snapshot(list, &disks);
    for (int j = 0; j < cnt; j++) {
        if (iter->dsk_iter_fn(disks[j]) == false) {
            break;
        }
    }
}

void
DiskInventory::dsk_foreach(DiskObjIter *iter,
                           DiskObjIter *arg,
                           ChainList   *list,
                           int          count)
{
    int           cnt;
    DiskObjArray  disks(count << 1);

    cnt = dsk_array_snapshot(list, &disks);
    for (int j = 0; j < cnt; j++) {
        if (iter->dsk_iter_fn(disks[j], arg) == false) {
            break;
        }
    }
}

}  // namespace fds
