/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PART_MGR_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PART_MGR_H_

#include "disk_obj_iter.h"

namespace fds
{
    class DiskPartMgr : public DiskObjIter
    {
        protected:
            ChainList    dsk_need_label;
            fds_mutex    dsk_mtx;

        public:
            DiskPartMgr();
            virtual ~DiskPartMgr();

            virtual bool dsk_iter_fn(DiskObject::pointer curr);
            virtual bool dsk_iter_fn(DiskObject::pointer curr, DiskObjIter *arg);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PART_MGR_H_
