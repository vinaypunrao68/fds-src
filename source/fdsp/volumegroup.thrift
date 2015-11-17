/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.replica

typedef i32             VolumeGroupVersion
typedef i64             VolumeGroupId

/* Header that must be part of every replica group io request */
struct VolumeIoHdr {
    1: required VolumeGroupVersion		version;

    /* Id to identify the replication group.  In case of a volume group it's volume id
     * In case of token it's the token id.
     */
    2: required VolumeGroupId			groupId;

    3: required svc_types.SvcUuid		svcUuid;
    /* Operation Id.  Incremented for every io */
    4: required i64				opId;
    /* Commit Id. This is incremented for every commit style io */
    5: required i64				commitId;
    6: i64					txId;
}

struct VolumeGroupInfo {
    1: required VolumeGroupId			groupId;
    2: required VolumeGroupVersion		version;
    3: list<svc_types.SvcUuid>			functionalReplicas;
    4: list<svc_types.SvcUuid>			nonfunctionalReplicas;
}

/* Message to update information about a replica group */
struct VolumeGroupInfoUpdateCtrlMsg {
    1: required VolumeGroupInfo	group;
}

/* Message to sent from Replica to Volumegroup coordinator to add non-functional replica into
 * the group
 */
struct AddToVolumeGroupCtrlMsg {
    1: required VolumeGroupId		groupId;
    2: required svc_types.SvcUuid	svcUuid;
    3: required i64			lastCommitId;
    4: required i64			lastOpId;
}

/* Response message from Volumegroup coordinator to replica */
struct AddToVolumeGroupRespCtrlMsg {
    /* Upto what commit the replica is missing */
    1: required i64			syncpointCommitId;
    /* Current replica group */
    2: required VolumeGroupInfo		group;
}

enum VolumeState {
    VOLUME_UNINIT,
    VOLUME_INITING,
    VOLUME_FUNCTIONAL,
    VOLUME_NONFUNCTIONAL_BEGIN,
    VOLUME_DOWN,
    VOLUME_SYNC_STARTED,
    VOLUME_SYNC_IN_PROGRESS,
    VOLUME_NONFUNCTIONAL_END
}

/* Notification from replica to any entitiy in the domain of the current state */
struct VolumeStateUpdateInfoCtrlMsg {
    1: required VolumeGroupId		groupId;
    2: required VolumeGroupVersion	version;
    /* Id of the replica sending this message */
    3: required svc_types.SvcUuid	svcUuid;
    4: required VolumeState		state; 
}

/* Query message to ge the 
struct GetVolumeStateCtrlMsg {
    1: required VolumeGroupId		groupId;
    2: required svc_types.SvcUuid	svcUuid;
}

/* Header to include when sending sync messages */
struct VolumeSyncEntryHdr {
    1: required VolumeGroupId		groupId;
    2: required svc_types.SvcUuid	svcUuid;
    3: required i64			commitId;
}

struct StartTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
}

struct UpdateTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
    2: map<string, string>		kvPairs;
}

struct CommitTxMsg {
    1: required VolumeIoHdr		volumeIoHdr;
}

struct SyncPullLogEntriesMsg {
    1: required VolumeGroupId		groupId;
    2: VolumeGroupVersion		lastCommitVersion;
    3: required i64			startCommitId;
    4: i64				endCommitId;
}

struct SyncPullLogEntriesRespMsg {
    1: required i64			startCommitId;
    2: list<binary>			entries;
}
