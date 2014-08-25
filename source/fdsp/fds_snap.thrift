/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "FDSP.thrift"
namespace cpp FDS_ProtocolInterface

/*
 * snapshot  and clone CLI interface  structures 
 */
struct SnapshotPolicyMsg {
     1: string policyName,
     2: i64  snapFrequency,
     3: i64  retensionTime,
     4: i64  timeOfDay,
     5: i64  dayOfMonth
}

struct CreateSnapshot {
     1: string volName,
     2: string snapPolicyName
}  

struct deleteSnapshot {
     1: string snapName  /* snap name is system generated, list and provide one*/
}

struct attachVolSnapPloicy {
     1: string volName,
     2: string snapPolicyName
}

struct detachVolSnapPloicy {
     1: string volName,
     2: string snapPolicyName
}

struct CreateVolumeClone {
     1: string volName,
     2: string cloneName,
     3: string clonePolicyName /* this is same as volume policy  structure */
}  

struct DeleteVolumeClone {
     1: string volName,
     2: string cloneName
}

struct RestoreVolumeSnap {
     1: string volName,
     2: string snapName  /* snap name is system generated, list and provide one*/
}

struct listVolumeSnap {
     1: string volName
}
