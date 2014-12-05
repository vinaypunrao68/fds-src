/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_LABEL_OP_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_LABEL_OP_H_

#include "disk_obj_iter.h"

namespace fds
{
    typedef enum
    {
        DISK_LABEL_READ  = 0,
        DISK_LABEL_WRITE = 1,
        DISK_LABEL_MAX
    } dlabel_op_e;

    /**
     * Switch board to route the main iterator function through disk inventory to the
     * desginated function (e.g. read/write)
     */
    class DiskLabelOp : public DiskObjIter
    {
        protected:
            dlabel_op_e     dl_op;
            DiskLabelMgr   *dl_mgr;

        public:
            explicit DiskLabelOp(dlabel_op_e op, DiskLabelMgr *mgr);
            virtual ~DiskLabelOp();

            virtual bool dsk_iter_fn(DiskObject::pointer curr);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_LABEL_OP_H_
