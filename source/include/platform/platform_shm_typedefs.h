/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_SHM_TYPEDEFS_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_SHM_TYPEDEFS_H_

namespace fds
{
    namespace fpi = FDS_ProtocolInterface;
    typedef fpi::FDSP_NodeState FdspNodeState;
    typedef fpi::FDSP_MgrIdType FdspNodeType;
    typedef fpi::FDSP_RegisterNodeTypePtr FdspNodeRegPtr;

    typedef enum
    {
        NODE_SHM_EMPTY          = 0,
        NODE_SHM_ACTIVE         = 0xfeedcafe
    } node_shm_state_e;

#define SHM_INV_FMT         "/0x%llx-%d"
#define SHM_QUEUE_FMT       "/0x%llx-rw-%d"

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_SHM_TYPEDEFS_H_
