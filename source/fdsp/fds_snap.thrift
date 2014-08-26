/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "FDSP.thrift"
namespace cpp FDS_ProtocolInterface

/*
 * snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicyMsg {
    1: string policyName,   /* Name of the policy, should be unique */
    2: string recurrenceRule, /* Recurrence rule as per iCal format */
    3: i64 retentionTime    /* Retention time in seconds */
}

struct SnapshotPolicyRespMsg {
    1: i64 id   /* id of newly created policy */
}

struct AttachVolumeToSnapshotPolicyMsg {
    1: string volName,
    2: string snapshotPolicyName
}

struct AttachVolumeToSnapshotPolicyRespMsg {
    1: i64 id
}

struct DetachVolumeFromSnapshotPolicyMsg {
    1: string volName,
    2: string snapshotPolicyName
}

struct DetachVolumeFromSnapshotPolicyRespMsg {
}

struct ListSnapshotForVolumeMsg {
    1: string volName
}

struct ListSnapshotForVolumeRespMsg {
    1: i64 id,
    2: string snapshotName,
    3: i64 timestamp
}

struct RestoreVolumeFromSnapshotMsg {
    1: string volName,
    2: string snapshotName
}

struct RestoreVolumeFromSnapshotRespMsg {
}

/* From OM => DM */
struct CreateSnapshotMsg {
    1: string volName,
    2: string snapshotPolicyName
}  

struct CreateSnapshotRespMsg {
    1: string snapshotName
}

struct DeleteSnapshotMsg {
    1: string snapshotName  /* snap name is system generated, list and provide one*/
}

struct DeleteSnapshotRespMsg {
}

struct CreateVolumeCloneMsg {
     1: string volName,
     2: string cloneName,
     3: string clonePolicyName /* this is same as volume policy  structure */
}  

struct CreateVolumeCloneRespMsg {
}

