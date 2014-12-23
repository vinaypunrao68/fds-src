/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_WORK_TYPE_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_WORK_TYPE_H_

namespace fds
{
    typedef enum
    {
        NWL_NO_OP                = 0,
        NWL_ADMIT_NODE           = 1,
        NWL_SYNC_START           = 2,
        NWL_SYNC_DONE            = 3,
        NWL_SYNC_CLOSE           = 4,
        NWL_MAX_OPS
    } node_work_type_e;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_WORK_TYPE_H_
