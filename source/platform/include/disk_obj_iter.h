/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_OBJ_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_OBJ_ITER_H_

#include "disk_object.h"

namespace fds
{
    class DiskObjIter
    {
        public:
            DiskObjIter();
            virtual ~DiskObjIter();

            /*
             * Return true to continue the iteration loop; false to quit.
             */
            virtual bool dsk_iter_fn(DiskObject::pointer curr) = 0;
            virtual bool dsk_iter_fn(DiskObject::pointer curr, DiskObjIter *cookie);
    };

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_OBJ_ITER_H_
