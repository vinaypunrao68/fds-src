/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_OP_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_OP_H_

#include "disk_obj_iter.h"
#include "disk_part_mgr.h"

namespace fds
{
    typedef enum
    {
        DISK_OP_NONE                    = 0,
        DISK_OP_CHECK_PARTITION         = 1,
        DISK_OP_DO_PARTITION            = 2,
        DISK_OP_MAX
    } disk_op_e;

    /**
     * Switch board to route the main iterator function to the designated function.
     */
    class DiskOp : public DiskObjIter
    {
        protected:
            disk_op_e           dsk_op;
            DiskPartMgr   *dsk_mgr;

        public:
            explicit DiskOp(disk_op_e op, DiskPartMgr *mgr);
            virtual ~DiskOp();

            virtual bool dsk_iter_fn(DiskObject::pointer curr);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_OP_H_
