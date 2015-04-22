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
using FdspCrtVolPtr = fpi::FDSP_CreateVolTypePtr;
using FdspDelVolPtr = fpi::FDSP_DeleteVolTypePtr;
using FdspModVolPtr = fpi::FDSP_ModifyVolTypePtr;

using FdspCrtPolPtr = fpi::FDSP_CreatePolicyTypePtr;
using FdspDelPolPtr = fpi::FDSP_DeletePolicyTypePtr;
using FdspModPolPtr = fpi::FDSP_ModifyPolicyTypePtr;

/*
 * NOTE: AttVolCmd is the command in the config plane, received at OM from CLI/GUI.
 * AttVol is the attach vol message sent from the OM to the HVs in the control plane.
 */
using FdspAttVolPtr = fpi::FDSP_AttachVolTypePtr;
using FdspAttVolCmdPtr = fpi::FDSP_AttachVolCmdTypePtr;

using FdspVolDescPtr = fpi::FDSP_VolumeDescTypePtr;
using FdspCrtDomPtr = fpi::FDSP_CreateDomainTypePtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_TYPEDEFS_H_
