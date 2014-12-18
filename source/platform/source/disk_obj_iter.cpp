/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "disk_obj_iter.h"

namespace fds
{
    DiskObjIter::DiskObjIter()
    {
    }

    DiskObjIter::~DiskObjIter()
    {
    }

    bool DiskObjIter::dsk_iter_fn(DiskObject::pointer curr, DiskObjIter *cookie)
    {
        return dsk_iter_fn(curr);
    }

}  // namespace fds
