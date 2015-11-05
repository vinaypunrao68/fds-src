/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.replica

typedef i32             ReplicaGroupVersion
typedef i64             ReplicaGroupId

/* Header that must be part of every replica group io request */
struct ReplicaIoHdr {
    1: required ReplicaGroupVersion		version;

    /* Id to identify the replication group.  In case of a volume group it's volume id
     * In case of token it's the token id.
     */
    2: required ReplicaGroupId			groupId;

    3: required common.SvcUuid			svcUuid;
    /* Operation Id.  Incremented for every io */
    4: required i64				opId;
    /* Commit Id. This is incremented for every commit style io */
    5: required i64				commitId;
}

struct ReplicaGroupInfo {
    1: required ReplicaGroupId			groupId;
    2: required ReplicaGroupVersion		version;
    3: list<common.SvcUuid>			functionalReplicas;
    4: list<common.SvcUuid>			nonfunctionalReplicas;
}

/* Message to update information about a replica group */
struct ReplicaGroupInfoUpdateCtrlMsg {
    1: required ReplicaGroupInfo	group;
}

/* Message to sent from Replica to Replicagroup coordinator to add non-functional replica into
 * the group
 */
struct AddToReplicaGroupCtrlMsg {
    1: required ReplicaGroupId		groupId;
    2: required common.SvcUuid		svcUuid;
    3: required i64			lastCommitId;
    4: required i64			lastOpId;
}

/* Response message from Replicagroup coordinator to replica */
struct AddToReplicaGroupRespCtrlMsg {
    /* Upto what commit the replica is missing */
    1: required i64			syncpointCommitId;
    /* Current replica group */
    2: required ReplicaGroupInfo	group;
}

enum ReplicaState {
    REPLICA_UNINIT,
    REPLICA_FUNCTIONAL,
    REPLICA_NONFUNCTIONAL_BEGIN,
    REPLICA_DOWN,
    REPLICA_SYNC_STARTED,
    REPLICA_SYNC_IN_PROGRESS,
    REPLICA_NONFUNCTIONAL_END
}

/* Notification from replica to any entitiy in the domain of the current state */
struct ReplicaStateUpdateInfoCtrlMsg {
    1: required ReplicaGroupId		groupId;
    2: required ReplicaGroupVersion	version;
    /* Id of the replica sending this message */
    3: required common.SvcUuid		svcUuid;
    4: required ReplicaState		state; 
}

/* Query message to ge the 
struct GetReplicaStateCtrlMsg {
    1: required ReplicaGroupId		groupId;
    2: required common.SvcUuid		svcUuid;
}

/* Header to include when sending sync messages */
struct ReplicaSyncEntryHdr {
    1: required ReplicaGroupId		groupId;
    2: required common.SvcUuid		svcUuid;
    3: required i64			commitId;
}
