/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PRINT_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PRINT_ITER_H_

#include "disk_obj_iter.h"

namespace fds
{
    class DiskPrintIter : public DiskObjIter
    {
        public:
            bool dsk_iter_fn(DiskObject::pointer disk)
            {
                LOGNORMAL << PmDiskObj::dsk_cast_ptr(disk);
                return true;
            }
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PRINT_ITER_H_
