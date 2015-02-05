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
// #include <fds_resource.h>
#include <fdsp/FDSP_types.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {

class ResourceUUID;
class UuidHash;

typedef std::string NodeStrName;

typedef ResourceUUID NodeUuid;

typedef fds_uint64_t VersionNumber;
typedef std::unordered_set<NodeUuid, UuidHash> NodeUuidSet;

typedef std::string fds_node_name_t;
typedef FDS_ProtocolInterface::FDSP_MgrIdType fds_node_type_t;
typedef FDS_ProtocolInterface::FDSP_NodeState FdspNodeState;

typedef FDS_ProtocolInterface::FDSP_MsgHdrTypePtr     FdspMsgHdrPtr;
typedef FDS_ProtocolInterface::FDSP_CreateVolTypePtr  FdspCrtVolPtr;
typedef FDS_ProtocolInterface::FDSP_DeleteVolTypePtr  FdspDelVolPtr;
typedef FDS_ProtocolInterface::FDSP_ModifyVolTypePtr  FdspModVolPtr;

typedef FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr FdspCrtPolPtr;
typedef FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr FdspDelPolPtr;
typedef FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr FdspModPolPtr;

/*
 * NOTE: AttVolCmd is the command in the config plane, received at OM from CLI/GUI.
 * AttVol is the attach vol message sent from the OM to the HVs in the control plane.
 */
typedef FDS_ProtocolInterface::FDSP_AttachVolTypePtr        FdspAttVolPtr;
typedef FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr     FdspAttVolCmdPtr;
typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr     FdspRegNodePtr;
typedef FDS_ProtocolInterface::FDSP_NotifyVolTypePtr        FdspNotVolPtr;
typedef FDS_ProtocolInterface::FDSP_TestBucketPtr           FdspTestBucketPtr;
typedef FDS_ProtocolInterface::FDSP_MigrationStatusTypePtr  FdspMigrationStatusPtr;

typedef FDS_ProtocolInterface::FDSP_VolumeDescTypePtr FdspVolDescPtr;
typedef FDS_ProtocolInterface::FDSP_PolicyInfoTypePtr FdspPolInfoPtr;
typedef FDS_ProtocolInterface::FDSP_CreateDomainTypePtr  FdspCrtDomPtr;
typedef FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr FdspGetDomStatsPtr;
typedef FDS_ProtocolInterface::FDSP_RemoveServicesTypePtr FdspRmServicesPtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_TYPEDEFS_H_
