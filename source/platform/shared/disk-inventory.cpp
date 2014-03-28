/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/disk-inventory.h>

namespace fds {

DiskObj::DiskObj() : Resource(ResourceUUID(0)) {}
DiskObj::~DiskObj() {}

DiskInventory::DiskInventory() : RsContainer(), dsk_count(0) {}
DiskInventory::~DiskInventory() {}

Resource *
DiskInventory::rs_new(const ResourceUUID &uuid)
{
    return new DiskObj();
}

// dsk_foreach
// -----------
//
void
DiskInventory::dsk_foreach(DiskObjIter *iter)
{
    int           cnt;
    ChainIter     i;
    ChainList    *l;
    DiskObjArray  disks(dsk_count << 1);

    cnt = 0;
    l = &dsk_hdd;

    rs_mtx.lock();
    for (l->chain_iter_init(&i); !l->chain_iter_term(i); l->chain_iter_next(&i)) {
        disks[cnt++] = l->chain_iter_current<DiskObj>(i);
    }
    rs_mtx.unlock();

    for (int j = 0; j < cnt; j++) {
        if (iter->dsk_iter_fn(disks[j]) == false) {
            break;
        }
    }
}

}  // namespace fds
