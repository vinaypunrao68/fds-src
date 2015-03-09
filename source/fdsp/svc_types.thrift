/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

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

  /** DM messages. */
  CtrlNotifyDMTCloseTypeId                  = 2081;
  CtrlNotifyDMTUpdateTypeId                 = 2082;
  CtrlNotifyDMAbortMigrationTypeId          = 2083;
  CtrlDMMigrateMetaTypeId                   = 2084;

  /** OM-> AM messages. */
  CtrlNotifyThrottleTypeId                  = 2101;
  CtrlNotifyQoSControlTypeId                = 2102;

  /** AM-> OM */
  CtrlTestBucketTypeId                      = 3000;
  CtrlGetBucketStatsTypeId                  = 3001;
  CtrlCreateBucketTypeId                    = 3002;
  CtrlDeleteBucketTypeId                    = 3003;
  CtrlModifyBucketTypeId                    = 3004;

  /** Svc -> OM */
  CtrlSvcEventTypeId                        = 9000;
  CtrlTokenMigrationAbortTypeId             = 9001;

  /** SM Type Ids*/
  GetObjectMsgTypeId                        = 10000;
  GetObjectRespTypeId                       = 10001;
  PutObjectMsgTypeId                        = 10002;
  PutObjectRspMsgTypeId                     = 10003;
  DeleteObjectMsgTypeId                     = 10004;
  DeleteObjectRspMsgTypeId                  = 10005;
  AddObjectRefMsgTypeId                     = 10006;
  AddObjectRefRspMsgTypeId                  = 10007;
  ShutdownMODMsgTypeId                      = 10008;

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
  SetVolumeMetaDataMsgTypeId;
  SetVolumeMetaDataRspMsgTypeId;
  GetVolumeMetaDataMsgTypeId;
  GetVolumeMetaDataRspMsgTypeId;
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
  ReloadVolumeMsgTypeId;
  ReloadVolumeRspMsgTypeId
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
  3: required i32               msg_src_id;
  /** Sender's Uuid */
  4: required common.SvcUuid    msg_src_uuid;
  /** Destination Uuid */
  5: required common.SvcUuid    msg_dst_uuid;
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