/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */
/**
 *
 */

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

const string INITIAL_XDI_VERSION = "0_8"
const string CURRENT_XDI_VERSION = INITIAL_XDI_VERSION

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

enum BlobListOrder {
    UNSPECIFIED = 0,
    LEXICOGRAPHIC,
    BLOBSIZE
}

enum ErrorCode {
    INTERNAL_SERVER_ERROR       = 0,
    MISSING_RESOURCE,
    BAD_REQUEST,
    RESOURCE_ALREADY_EXISTS,
    RESOURCE_NOT_EMPTY,
    SERVICE_NOT_READY,
    SERVICE_SHUTTING_DOWN,
    TIMEOUT,
}

enum PatternSemantics {
    PCRE,
    PREFIX,
    PREFIX_AND_DELIMITER
}

exception ApiException {
    1: string message,
    2: ErrorCode errorCode,
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct BlobDescriptor {
     1: required string name,
     2: required i64 byteCount,
     3: required map<string, string> metadata
}

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

struct VolumeAccessMode {
  1: optional bool can_write = true;
  2: optional bool can_cache = true;
}
