/*
 * Copyright 2016 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_MNT_PT_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_MNT_PT_ITER_H_

#include "disk_obj_iter.h"
#include "platform_disk_obj.h"

namespace fds
{
    class DiskMntPtIter : public DiskObjIter
    {
        private:
            PmDiskObj::pointer prev = NULL;

        public:
            bool dsk_iter_fn(DiskObject::pointer disk);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_MNT_PT_ITER_H_
