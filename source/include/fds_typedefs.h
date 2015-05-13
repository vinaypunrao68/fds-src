/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_TYPEDEFS_H_
#define SOURCE_INCLUDE_FDS_TYPEDEFS_H_

/**
 * All system wide type definitions.
 * Please include the appropriate header files
 */

#include <string>
#include <vector>
#include <unordered_set>
#include <fds_types.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/config_api_types.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {

class ResourceUUID;
class UuidHash;

using NodeStrName = std::string;

using NodeUuid = ResourceUUID;

using VersionNumber = fds_uint64_t;
using NodeUuidSet = std::unordered_set<NodeUuid, UuidHash>;

using fds_node_name_t = NodeStrName;
using fds_node_type_t = fpi::FDSP_MgrIdType;
using FdspNodeState = fpi::FDSP_NodeState;

using FdspMsgHdrPtr = fpi::FDSP_MsgHdrTypePtr;
using FdspCrtVolPtr = apis::FDSP_CreateVolTypePtr;
using FdspDelVolPtr = apis::FDSP_DeleteVolTypePtr;
using FdspModVolPtr = apis::FDSP_ModifyVolTypePtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_TYPEDEFS_H_
