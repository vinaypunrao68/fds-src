/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__
include "common.thrift"
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.apis

/*
 * snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicy {
    1: i64 id;
    2: string policyName,   /* Name of the policy, should be unique */
    3: string recurrenceRule, /* Recurrence rule as per iCal format */
    4: i64 retentionTimeSeconds,    /* Retention time in seconds */
    5: common.ResourceState state,
}

struct Snapshot {
    1:i64 snapshotId,
    2:string snapshotName,
    3:i64 volumeId,
    4:i64 snapshotPolicyId,
    5:i64 creationTimestamp,
    6:i64 retentionTimeSeconds
    7:common.ResourceState state,
}

#endif // __SNAPSHOT_H__