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

struct StorCapMsg {
    1: i32                    disk_iops_max,
    2: i32                    disk_iops_min,
    3: double                 disk_capacity,
    4: i32                    disk_latency_max,
    5: i32                    disk_latency_min,
    6: i32                    ssd_iops_max,
    7: i32                    ssd_iops_min,
    8: double                 ssd_capacity,
    9: i32                    ssd_latency_max,
    10: i32                   ssd_latency_min,
    11: i32                   ssd_count,
    12: i32                   disk_type,
    13: i32                   disk_count,
}

struct SvcVer {
    1: required i16           ver_major,
    2: required i16           ver_minor,
}

/* A detailed list of blob stats. */
typedef list<BlobDescriptor> BlobDescriptorListType
