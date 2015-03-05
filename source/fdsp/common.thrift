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
