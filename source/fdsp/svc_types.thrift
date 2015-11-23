/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.svc.types

/*
 * Service/domain identifiers.
 */
struct SvcUuid {
    1: required i64           svc_uuid,
}

struct DomainID {
    1: required SvcUuid       domain_id,
    2: required string        domain_name,
}

/*
 * Node storage capability message format.
 */
struct SvcID {
    1: required SvcUuid       svc_uuid,
    2: required string        svc_name,
}

enum NodeSvcMask {
    NODE_SVC_SM       = 0x0001,
    NODE_SVC_DM       = 0x0002,
    NODE_SVC_AM       = 0x0004,
    NODE_SVC_OM       = 0x0008,
    NODE_SVC_GENERAL  = 0x1000
}

struct SvcVer {
    1: required i16           ver_major,
    2: required i16           ver_minor,
}

/**
 * Describes an object id.
 * TODO(Andrew): Should this just be a typedef?
 */
struct FDS_ObjectIdType {
  1: string  digest
}

enum FDSP_NotifyVolFlag {
  FDSP_NOTIFY_VOL_NO_FLAG,
  FDSP_NOTIFY_VOL_CHECK_ONLY  // for delete vol -- only check if objects in volume
}

enum FDSP_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd,
     FDS_Node_Discovered,
     FDS_Start_Migration,
     FDS_Node_Standby
}

enum FDSP_MgrIdType {
    FDSP_PLATFORM       = 0x0,
    FDSP_STOR_MGR       = 0x1,
    FDSP_DATA_MGR       = 0x2,
    FDSP_ACCESS_MGR     = 0x3,
    FDSP_ORCH_MGR       = 0x4,
    FDSP_CLI_MGR        = 0x5,
    FDSP_OMCLIENT_MGR   = 0x6,
    FDSP_MIGRATION_MGR  = 0x7,
    FDSP_PLATFORM_SVC   = 0x8,
    FDSP_METASYNC_MGR   = 0x9,
    FDSP_TEST_APP       = 0xa,
    FDSP_CONSOLE        = 0xb,
    FDSP_INVALID_SVC    = 0xc
}

/**
 * Despite its name, this structure captures information about Services,
 * which, of course, includes a good bit of Node information as well.
 */
struct FDSP_Node_Info_Type {
  1: i32            node_id,
  2: FDSP_NodeState node_state,
  3: FDSP_MgrIdType node_type, /* Actually, type of Service - SM/DM/AM */
  4: string         node_name, /* node identifier - string */
  5: i64            ip_hi_addr, /* IP V6 high address */
  6: i64            ip_lo_addr, /* IP V4 address of V6 low address of the node */
  7: i32            control_port, /* Port number to contact for control messages */
  8: i32            data_port, /* Port number to send datapath requests */
  9: i32            migration_port, /* Migration service port */
  10: i64           node_uuid, /* UUID of the node */
  11: i64           service_uuid, /* UUID of the service */
  12: string        node_root, /* node root - string */
  13: i32           metasync_port, /* Migration service port */
}

typedef list<FDSP_Node_Info_Type> Node_Info_List_Type

struct FDSP_Uuid {
  1: i64          uuid,
}

/**
 *    A throttle level of X.Y (e.g, 5.6) means we should
 *    1. throttle all traffic for priorities greater than X (priorities 6,7,8,9
 *       for a 5.6 throttle level) to their guaranteed min rate,
 *    2. allow all traffic for priorities less than X (priorities 0,1,2,3,4 for
 *       a 5.6 throttle level) to go up till their max rate limit,
 *    3. traffic for priority X to a rate = min_rate + Y/10 * (max_rate - min_rate).
 *       (A volume that has a min rate of 300 IOPS and max rate of 600 IOPS will
 *       be allowed to pump at 480 IOPS when throttle level is 5.6).
 *
 *    A throttle level of 0 indicates all volumes should be limited at their min_iops rating.
 *    A negative throttle level of -X means all volumes should be throttled at (10-X)/10 of their min iops.
 */
struct FDSP_ThrottleMsgType {
  /** Domain Identifier. */
  1: i32    domain_id, /* Domain this throttle message is meant for */
  /** Throttle level. */
  2: double throttle_level, /* a real number between -10 and 10 */
}

enum FDSP_MediaPolicy {
  FDSP_MEDIA_POLICY_UNSET,                /* only used by cli or other client on modify to not change existing policy */
  FDSP_MEDIA_POLICY_HDD,                  /* always on hdd */
  FDSP_MEDIA_POLICY_SSD,                  /* always on ssd */
  FDSP_MEDIA_POLICY_HYBRID,               /* either hdd or ssd, but prefer ssd */
  FDSP_MEDIA_POLICY_HYBRID_PREFCAP        /* either on hdd or ssd, but prefer hdd */
}

enum FDSP_VolType {
  FDSP_VOL_S3_TYPE,
  FDSP_VOL_BLKDEV_TYPE
  FDSP_VOL_NFS_TYPE
  FDSP_VOL_ISCSI_TYPE
}

enum ResourceState {
  Unknown,
  Loading, /* resource is loading or in the middle of creation */
  Created, /* resource has been created */
  Active,  /* resource activated - ready to use */
  Offline, /* resource is offline - will come back later */
  MarkedForDeletion, /* resource will be deleted soon. */
  Deleted, /* resource is gone now and will not come back*/
  InError,  /*in known erroneous state*/
}

struct FDSP_VolumeDescType {
  1: required string            vol_name,  /* Name of the volume */
  2: i32                        tennantId,  // Tennant id that owns the volume
  3: i32                        localDomainId,  // Local domain id that owns vol
  4: required i64               volUUID,

  // Basic operational properties
  5: required FDSP_VolType      volType,
  6: i32                        maxObjSizeInBytes,
  7: required double            capacity,

  // Other policies
  8: i32                        volPolicyId,
  9: i32                        placementPolicy,  // Can change placement policy

  // volume policy details
  10: i64                       iops_assured, /* minimum (guaranteed) iops */
  11: i64                       iops_throttle, /* maximum iops */
  12: i32                       rel_prio, /* relative priority */
  13: required FDSP_MediaPolicy mediaPolicy   /* media policy */

  14: bool                      fSnapshot,
  15: ResourceState      state,
  16: i64                       contCommitlogRetention,
  17: i64                       srcVolumeId,
  18: i64                       timelineTime,
  19: i64                       createTime
  // 20: Removed.
}

struct FDSP_PolicyInfoType {
  1: string                 policy_name,    /* Name of the policy */
  2: i32                    policy_id,      /* policy id */
  3: double                 iops_assured,   /* minimum (guaranteed) iops */
  4: double                 iops_throttle,  /* maximum iops */
  5: i32                    rel_prio,       /* relative priority */
}

/**
 * Descriptor for a snapshot. Describes it name and
 * policy information.
 */
struct Snapshot {
    1:i64 snapshotId,
    2:string snapshotName,
    3:i64 volumeId,
    4:i64 snapshotPolicyId,
    5:i64 creationTimestamp,
    6:i64 retentionTimeSeconds,
    7:ResourceState state,
    8:i64 timelineTime,
}

/* ------------------------------------------------------------
   List of all FDSP message types that passed between fds services.  Typically all these
   types are for async messages.

   Process for adding new type:
   1. DON'T CHANGE THE ORDER.  ONLY APPEND.
   2. Add the type enum in appropriate service section.  Naming convection is
      Message type + "TypeId".  For example PutObjectMsg is PutObjectMsgTypeId.
      This helps us to use macros in c++.
   3. Add the type defition in the appropriate service .thrift file
   4. Register the serializer/deserializer in the appropriate service (.cpp/source) files
   ------------------------------------------------------------*/
enum  FDSPMsgTypeId {
  UnknownMsgTypeId                          = 0;

  /** Common Service Types */
  NullMsgTypeId                             = 10;
  EmptyMsgTypeId                            = 11;
  StatStreamMsgTypeId                       = 12;

  /** File Transfer **/
  FileTransferMsgTypeId                     = 900;
  FileTransferRespMsgTypeId;
  FileTransferVerifyMsgTypeId;
  FileTransferVerifyRespMsgTypeId;

  /** Node/service event messages. */
  NodeSvcInfoTypeId                         = 1000;
  UuidBindMsgTypeId                         = 1001;
  DomainNodesTypeId                         = 1002;
  NodeInfoMsgTypeId                         = 1003;
  NodeInfoMsgListTypeId                     = 1004;
  NodeQualifyTypeId                         = 1005;
  NodeUpgradeTypeId                         = 1006;
  NodeRollbackTypeId                        = 1007;
  NodeIntegrateTypeId                       = 1008;
  NodeDeployTypeId                          = 1009;
  NodeFunctionalTypeId                      = 1010;
  NodeDownTypeId                            = 1011;
  NodeEventTypeId                           = 1012;
  NodeWorkItemTypeId                        = 1013;
  PhaseSyncTypeId                           = 1014;
  UpdateSvcMapMsgTypeId                     = 1015;
  GetSvcMapMsgTypeId                        = 1016;
  GetSvcMapRespMsgTypeId                    = 1017;
  GetSvcStatusMsgTypeId                     = 1018;
  GetSvcStatusRespMsgTypeId                 = 1019;
  /* @Deprecated by Node & Service control messages */
  ActivateServicesMsgTypeId                 = 1020;
  DeactivateServicesMsgTypeId               = 1021;
  /* Node & Service control messages */
  NotifyEventMsgTypeId                      = 1022;
  NotifyStartServiceMsgTypeId               = 1023;
  NotifyStopServiceMsgTypeId                = 1024;
  NotifyAddServiceMsgTypeId                 = 1025;
  NotifyRemoveServiceMsgTypeId              = 1036;
  GetAllVolumeDescriptorsTypeId             = 1037;

  /** Volume messages; common for AM, DM, SM. */
  CtrlNotifyVolAddTypeId                    = 2020;
  CtrlNotifyVolRemoveTypeId                 = 2021;
  CtrlNotifyVolModTypeId                    = 2022;
  CtrlNotifySnapVolTypeId                   = 2023;
  CtrlTierPolicyTypeId                      = 2024;
  CtrlTierPolicyAuditTypeId                 = 2025;
  CtrlStartHybridTierCtrlrMsgTypeId         = 2026;

  /** SM messages. */
  CtrlStartMigrationTypeId                  = 2040;
  CtrlNotifyScavengerTypeId                 = 2041;
  CtrlQueryScavengerStatusTypeId            = 2042;
  CtrlQueryScavengerStatusRespTypeId        = 2043;
  CtrlQueryScavengerProgressTypeId          = 2044;
  CtrlQueryScavengerProgressRespTypeId      = 2045;
  CtrlSetScavengerPolicyTypeId              = 2046;
  CtrlSetScavengerPolicyRespTypeId          = 2047;
  CtrlQueryScavengerPolicyTypeId            = 2048;
  CtrlQueryScavengerPolicyRespTypeId        = 2049;
  CtrlQueryScrubberStatusTypeId             = 2050;
  CtrlQueryScrubberStatusRespTypeId         = 2051;
  CtrlSetScrubberStatusTypeId               = 2052;
  CtrlSetScrubberStatusRespTypeId           = 2053;
  CtrlNotifyMigrationFinishedTypeId         = 2054;
  CtrlNotifyMigrationStatusTypeId           = 2055;
  CtrlNotifySMCheckTypeId                   = 2056;
  CtrlNotifySMCheckStatusTypeId             = 2057;
  CtrlNotifySMCheckStatusRespTypeId         = 2058

  CtrlNotifyDLTUpdateTypeId                 = 2060;
  CtrlNotifyDLTCloseTypeId                  = 2061;
  CtrlNotifySMStartMigrationTypeId          = 2062;
  CtrlObjectRebalanceFilterSetTypeId        = 2063;
  CtrlObjectRebalanceDeltaSetTypeId         = 2064;
  CtrlNotifySMAbortMigrationTypeId          = 2065;
  CtrlGetSecondRebalanceDeltaSetTypeId      = 2066;
  CtrlGetSecondRebalanceDeltaSetRspTypeId   = 2067;
  CtrlFinishClientTokenResyncMsgTypeId      = 2068;
  CtrlFinishClientTokenResyncRspMsgTypeId   = 2069;

  /** DM messages. */
  CtrlNotifyDMTCloseTypeId                  = 2081;
  CtrlNotifyDMTUpdateTypeId                 = 2082;
  CtrlNotifyDMAbortMigrationTypeId          = 2083;
  CtrlDMMigrateMetaTypeId                   = 2084;

  /** OM-> AM messages. */
  CtrlNotifyThrottleTypeId                  = 2101;
  CtrlNotifyQoSControlTypeId                = 2102;

  /** AM-> OM */
  GetVolumeDescriptorTypeId                 = 3000;
  CtrlGetBucketStatsTypeId                  = 3001;
  GetVolumeDescriptorRespTypeId             = 3002;

  /** Svc -> OM */
  CtrlSvcEventTypeId                        = 9000;
  CtrlTokenMigrationAbortTypeId             = 9001;
  SvcStateChangeRespTypeId                  = 9002;
  
  /** SM Type Ids*/
  GetObjectMsgTypeId                        = 10000;
  GetObjectRespTypeId                       = 10001;
  PutObjectMsgTypeId                        = 10002;
  PutObjectRspMsgTypeId                     = 10003;
  DeleteObjectMsgTypeId                     = 10004;
  DeleteObjectRspMsgTypeId                  = 10005;
  AddObjectRefMsgTypeId                     = 10006;
  AddObjectRefRspMsgTypeId                  = 10007;
  PrepareForShutdownMsgTypeId               = 10008;
  ActiveObjectsMsgTypeId;
  ActiveObjectsRspMsgTypeId;

  /** DM Type Ids */
  QueryCatalogMsgTypeId                     = 20000;
  QueryCatalogRspMsgTypeId;
  StartBlobTxMsgTypeId;
  StartBlobTxRspMsgTypeId;
  UpdateCatalogMsgTypeId;
  UpdateCatalogRspMsgTypeId;
  UpdateCatalogOnceMsgTypeId;
  UpdateCatalogOnceRspMsgTypeId;
  SetBlobMetaDataMsgTypeId;
  SetBlobMetaDataRspMsgTypeId;
  GetBlobMetaDataMsgTypeId;
  GetBlobMetaDataRspMsgTypeId;
  StatVolumeMsgTypeId;
  StatVolumeRspMsgTypeId;
  SetVolumeMetadataMsgTypeId;
  SetVolumeMetadataRspMsgTypeId;
  GetVolumeMetadataMsgTypeId;
  GetVolumeMetadataRspMsgTypeId;
  CommitBlobTxMsgTypeId;
  CommitBlobTxRspMsgTypeId;
  AbortBlobTxMsgTypeId;
  AbortBlobTxRspMsgTypeId;
  GetBucketMsgTypeId;
  GetBucketRspMsgTypeId;
  DeleteBlobMsgTypeId;
  DeleteBlobRspMsgTypeId;
  ForwardCatalogMsgTypeId;
  ForwardCatalogRspMsgTypeId;
  VolSyncStateMsgTypeId;
  VolSyncStateRspMsgTypeId;
  StatStreamRegistrationMsgTypeId;
  StatStreamRegistrationRspMsgTypeId;
  StatStreamDeregistrationMsgTypeId;
  StatStreamDeregistrationRspMsgTypeId;
  CreateSnapshotMsgTypeId;
  CreateSnapshotRespMsgTypeId;
  DeleteSnapshotMsgTypeId;
  DeleteSnapshotRespMsgTypeId;
  CreateVolumeCloneMsgTypeId;
  CreateVolumeCloneRespMsgTypeId;
  GetDmStatsMsgTypeId;
  GetDmStatsMsgRespTypeId;
  ListBlobsByPatternMsgTypeId;
  ListBlobsByPatternRspMsgTypeId;
  OpenVolumeMsgTypeId;
  OpenVolumeRspMsgTypeId;
  CloseVolumeMsgTypeId;
  CloseVolumeRspMsgTypeId;
  ReloadVolumeMsgTypeId;
  ReloadVolumeRspMsgTypeId;
  CtrlNotifyDMStartMigrationMsgTypeId;
  CtrlNotifyDMStartMigrationRspMsgTypeId;
  CtrlNotifyInitialBlobFilterSetMsgTypeId;
  CtrlNotifyInitialBlobFilterSetRspMsgTypeId;
  CtrlNotifyDeltaBlobDescMsgTypeId;
  CtrlNotifyDeltaBlobDescRspMsgTypeId;
  CtrlNotifyDeltaBlobsMsgTypeId;
  CtrlNotifyDeltaBlobsRspMsgTypeId;
  RenameBlobMsgTypeId;
  RenameBlobRespMsgTypeId;
  CtrlNotifyTxStateMsgTypeId;
  CtrlNotifyTxStateRspMsgTypeId;

  /** Health Status */
  NotifyHealthReportTypeId                  = 100000;
  HeartbeatMessageTypeId                    = 100001;
  EventMessageTypeId;
}

/**
 * Service status.
 */
enum ServiceStatus {
    SVC_STATUS_INVALID      = 0x0000;
    SVC_STATUS_ACTIVE       = 0x0001;
    SVC_STATUS_INACTIVE     = 0x0002;
/*
 * We really need some way to determine that a "PM service" is in a
 * "discovered" state to allow us to add it on first registration
 */
    SVC_STATUS_DISCOVERED   = 0x0003;
/*
 * When we shutdown or remove a node, we need a state to reflect
 * that while the PM is not in inactive state, it is not active either
 */
    SVC_STATUS_STANDBY      = 0x0004;
    SVC_STATUS_ADDED        = 0x0005;
    SVC_STATUS_STARTED      = 0x0006;
    SVC_STATUS_STOPPED      = 0x0007;
}

/* ------------------------------------------------------------
   SvcLayer Types
   ------------------------------------------------------------*/

/*
 * This message header is owned, controlled and set by the net service layer.
 * Application code treats it as opaque type.
 */
struct AsyncHdr {
  /** Checksum Integrity */
  1: required i32               msg_chksum;
  /** Message Type */
  2: required FDSPMsgTypeId     msg_type_id;
  /**  */
  3: required i64               msg_src_id;
  /** Sender's Uuid */
  4: required SvcUuid    msg_src_uuid;
  /** Destination Uuid */
  5: required SvcUuid    msg_dst_uuid;
  /**  */
  6: required i32               msg_code;
  /**  */
  7: optional i64               dlt_version = 0;
  /**  */
  8: i64                        rqSendStartTs;
  /**  */
  9: i64                        rqSendEndTs;
  /**  */
  10:i64                        rqRcvdTs;
  /**  */
  11:i64                        rqHndlrTs;
  /**  */
  12:i64                        rspSerStartTs;
  /**  */
  13:i64                        rspSendStartTs;
  /**  */
  14:i64                        rspRcvdTs;
}


struct FDSP_DLT_Data_Type {
  /**  */
  1: bool   dlt_type;
  /**  */
  2: binary dlt_data;
}

struct FDSP_DMT_Data_Type {
  /**  */
  1: bool   dmt_type;
  /**  */
  2: binary dmt_data;
}

/**
 * Service map info
 */
struct SvcInfo {
  1: required SvcID             svc_id;
  2: required i32               svc_port;
  3: required FDSP_MgrIdType    svc_type;
  4: required ServiceStatus     svc_status;
  5: required string            svc_auto_name;
  // TODO(Rao): We should make these required.  They aren't made required as of this writing
  // because it can break existing code.
  6: string                         ip;
  7: i32                            incarnationNo;
  8: string                         name;
  /* <key, value> properties */
  9: map<string, string>            props;
}

/*
 * --------------------------------------------------------------------------------
 * Node endpoint/service registration handshake
 * --------------------------------------------------------------------------------
 */

/*
 * @deprecated 06/09/2015
 */
struct FDSP_ActivateNodeType {
  1: FDSP_Uuid  node_uuid,
  2: string     node_name,          /* autogenerated name */
  3: bool       has_sm_service,     /* true if node runs sm service */
  4: bool       has_dm_service,     /* true if node runs dm service */
  5: bool       has_om_service,     /* true if node runs om service */
  6: bool       has_am_service      /* true if node runs am service */
}

/**
 * Activate Service
 * @deprecated 06/09/2015
 */
struct ActivateServicesMsg {
  1: FDSP_ActivateNodeType info;
}

/**
 * Bind service to Uuid.
 */
struct UuidBindMsg {
    1: required SvcID           svc_id;
    2: required string          svc_addr;
    3: required i32             svc_port;
    4: required SvcID           svc_node;
    5: required string          svc_auto_name;
    6: required FDSP_MgrIdType  svc_type;
}

/**
 * This becomes a control path message
 */
struct NodeSvcInfo {
    1: required SvcUuid         node_base_uuid,
    2: i32                      node_base_port,
    3: string                   node_addr,
    4: string                   node_auto_name,
    5: FDSP_NodeState           node_state;
    6: i32                      node_svc_mask,
    7: list<SvcInfo>            node_svc_list,
}

struct DomainNodes {
    1: required DomainID        dom_id,
    2: list<NodeSvcInfo>        dom_nodes,
}

struct StorCapMsg {
    1: i32                    node_iops_max,
    2: i32                    node_iops_min,
    3: double                 disk_capacity,
    4: double                 ssd_capacity,
    5: i32                    disk_type,
}

/**
 * Work item done by a node.
 */
struct NodeWorkItem {
    1: required i32                      nd_work_code,
    2: required DomainID          nd_dom_id,
    3: required SvcUuid           nd_from_svc,
    4: required SvcUuid           nd_to_svc,
}

struct NodeDeploy {
    1: required DomainID          nd_dom_id,
    2: required SvcUuid           nd_uuid,
    3: list<NodeWorkItem>                nd_work_item,
}

struct NodeDown {
    1: required DomainID          nd_dom_id,
    2: required SvcUuid           nd_uuid,
}

/**
 * Events emit from a node.
 */
struct NodeEvent {
    1: required DomainID          nd_dom_id,
    2: required SvcUuid           nd_uuid,
    3: required string                   nd_evt,
    4: string                            nd_evt_text,
}

struct NodeFunctional {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: required FDSPMsgTypeId                   nd_op_code,
    4: list<NodeWorkItem>                       nd_work_item,
}

/*
 * Node registration message format.
 */
struct NodeInfoMsg {
    1: required UuidBindMsg node_loc,
    2: required DomainID         node_domain,
    3: StorCapMsg                       node_stor,
    4: required i32                     nd_base_port,
    5: required i32                     nd_svc_mask,
    6: required bool                    nd_bcast,
}

struct NodeIntegrate {
    1: required DomainID          nd_dom_id,
    2: required SvcUuid           nd_uuid,
    3: bool                              nd_start_am,
    4: bool                              nd_start_dm,
    5: bool                              nd_start_sm,
    6: bool                              nd_start_om,
}

struct NodeVersion {
    1: required i32                      nd_fds_maj,
    2: required i32                      nd_fds_min,
    3: i32                               nd_fds_release,
    4: i32                               nd_fds_patch,
    5: string                            nd_java_ver,
}

/**
 * Qualify a node before admitting it to the domain.
 */
struct NodeQualify {
    1: required NodeInfoMsg              nd_info,
    2: required string                   nd_acces_token,
    3: NodeVersion                       nd_sw_ver,
    4: string                            nd_md5sum_pm,
    5: string                            nd_md5sum_sm,
    6: string                            nd_md5sum_dm,
    7: string                            nd_md5sum_am,
    8: string                            nd_md5sum_om,
}

/**
 * Notify node to upgrade/rollback SW version.
 */
struct NodeUpgrade {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: NodeVersion                              nd_sw_ver,
    4: required FDSPMsgTypeId   nd_op_code,
    5: required string                          nd_md5_chksum,
    6: string                                   nd_pkg_path,
}
