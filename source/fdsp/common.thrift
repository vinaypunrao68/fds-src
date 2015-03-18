/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

enum ResourceState {
  Unknown,
  Loading, /* resource is loading or in the middle of creation */
  Created, /* resource has been created */
  Active,  /* resource activated - ready to use */
  Offline, /* resource is offline - will come back later */
  MarkedForDeletion, /* resource will be deleted soon. */
  Deleted, /* resource is gone now and will not come back*/
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

struct FDSP_CreateVolType {
  1: string                  vol_name,
  2: FDSP_VolumeDescType     vol_info, /* Volume properties and attributes */
}

struct FDSP_DeleteVolType {
  1: string 		 vol_name,  /* Name of the volume */
  // i64    		 vol_uuid,
  2: i32			 domain_id,
}

struct FDSP_ModifyVolType {
  1: string 		 vol_name,  /* Name of the volume */
  2: i64		 vol_uuid,
  3: FDSP_VolumeDescType	vol_desc,  /* New updated volume descriptor */
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
