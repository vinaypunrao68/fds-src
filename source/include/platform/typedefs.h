/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_TYPEDEFS_H_
#define SOURCE_INCLUDE_PLATFORM_TYPEDEFS_H_

#include "fds_types.h"
#include "platform/fpi_namespace.h"

// Forward declarations
namespace FDS_ProtocolInterface
{
    class PlatNetSvcClient;
    class FDSP_OMControlPathReqClient;
}  // namespace FDS_ProtocolInterface

namespace fds
{
    typedef fpi::FDSP_RegisterNodeType FdspNodeReg;
    typedef fpi::FDSP_RegisterNodeTypePtr FdspNodeRegPtr;
    typedef fpi::FDSP_NodeState FdspNodeState;
    typedef fpi::FDSP_MgrIdType FdspNodeType;
    typedef fpi::FDSP_ActivateNodeTypePtr FdspNodeActivatePtr;

    typedef boost::shared_ptr<fpi::FDSP_OMControlPathReqClient>   NodeAgentCpOmClientPtr;

    const fds_uint32_t       NODE_DO_PROXY_ALL_SVCS =
                             (fpi::NODE_SVC_SM | fpi::NODE_SVC_DM | fpi::NODE_SVC_AM);
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_TYPEDEFS_H_
