/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_OP_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_OP_H_

#include "disk_obj_iter.h"

namespace fds
{
    class DiskCapabilitiesMgr;

    typedef enum
    {
        DISK_CAPABILITIES_READ  = 0,
        DISK_CAPABILITIES_MAX
    } dcapablitiy_op_e;

    /**
     * Switch board to route the main iterator function through disk inventory to the
     * desginated function (e.g. read/write)
     */

    class DiskCapabilityOp : public DiskObjIter
    {
        protected:
            dcapablitiy_op_e     dc_op;
            DiskCapabilitiesMgr *dc_mgr;

        public:
            explicit DiskCapabilityOp(dcapablitiy_op_e op, DiskCapabilitiesMgr *mgr);
            virtual ~DiskCapabilityOp();

            virtual bool dsk_iter_fn(DiskObject::pointer curr);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_OP_H_

