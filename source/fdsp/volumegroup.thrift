/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.replica

typedef i32             VolumeGroupVersion
typedef svc_types.ReplicaId VolumeGroupId

/* Header that must be part of every replica group io request */
struct VolumeIoHdr {
    1: required VolumeGroupVersion		version;

    /* Id to identify the replication group */
    2: required VolumeGroupId			groupId;

    3: required svc_types.SvcUuid		svcUuid;
    /* Operation Id.  Incremented for every io */
    4: required i64				opId;
    /* Commit Id. This is incremented for every commit style io */
    5: required i64				commitId;
    6: i64					txId;
}

struct VolumeGroupCoordinatorInfo {
    1: required svc_types.SvcUuid		id;
    2: required i32				version;
}

struct VolumeGroupInfo {
    1: required VolumeGroupId			groupId;
    2: required VolumeGroupVersion		version;
    /* Current coordinator op id */
    3: i64					lastOpId;
    /* Current coordinator commit id */
    4: i64					lastCommitId;
    5: list<svc_types.SvcUuid>			functionalReplicas;
    6: list<svc_types.SvcUuid>			nonfunctionalReplicas;
}

/* Message to update information about a replica group */
struct VolumeGroupInfoUpdateCtrlMsg {
    1: required VolumeGroupInfo	group;
}

/* Message to sent from Replica to Volumegroup coordinator to add non-functional replica into
 * the group
 */
struct AddToVolumeGroupCtrlMsg {
    1: svc_types.ResourceState		targetState;
    2: required VolumeGroupId		groupId;
    3: required i32			replicaVersion;
    4: required svc_types.SvcUuid	svcUuid;
    5: required i64			lastOpId;
    6: required i64			lastCommitId;
}

/* Response message from Volumegroup coordinator to replica */
struct AddToVolumeGroupRespCtrlMsg {
    /* Current replica group */
    1: required VolumeGroupInfo		group;
}

/* Message sent from Coordinator to OM to register the coordinator for a vol */
struct SetVolumeGroupCoordinatorMsg {
    1: VolumeGroupCoordinatorInfo	coordinator;
    2: i64                          volumeId;
}

/* BEGIN exposed for testing.  Will be removed */
struct StartTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
}

typedef map<string, string> 		TxUpdates
struct UpdateTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
    2: TxUpdates			kvPairs;
}

struct CommitTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
}

struct PullCommitLogEntriesMsg {
    1: required VolumeGroupId		groupId;
    2: VolumeGroupVersion		lastCommitVersion;
    3: required i64			startCommitId;
    4: i64				endCommitId;
}

struct PullCommitLogEntriesRespMsg {
    1: required i64			startCommitId;
    2: list<binary>			entries;
}
/* END exposed for testing.  Will be removed */

