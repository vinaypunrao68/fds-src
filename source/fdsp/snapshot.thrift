/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "FDSP.thrift"
namespace cpp FDS_ProtocolInterface

/*
 * snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicy {
    1: i64 id;
    2: string policyName,   /* Name of the policy, should be unique */
    3: string recurrenceRule, /* Recurrence rule as per iCal format */
    4: i64 retentionTimeSeconds    /* Retention time in seconds */
}

struct Snapshot {
    1:i64 id,
    2:string snapshotName,
    3:i64 volumeId,
    4:i64 policyId,
    5:i64 creationTimestamp
}

/* From OM => DM */
struct CreateSnapshotMsg {
    1:i64 volumeId,
    2:i64 snapshotPolicyId
}  

struct CreateSnapshotRespMsg {
    1:Snapshot snapshot
}

struct DeleteSnapshotMsg {
    1:i64 snapshotId
}

struct DeleteSnapshotRespMsg {
}

struct CreateVolumeCloneMsg {
     1:i64 volumeId,
     2:i64 cloneId,
     3:i64 fdsp_PolicyInfoId
}  

struct CreateVolumeCloneRespMsg {
}

