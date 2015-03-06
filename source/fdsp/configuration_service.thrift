/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

enum MediaPolicy {
    SSD_ONLY    = 0,
    HDD_ONLY    = 1,
    HYBRID_ONLY = 3,
}

enum VolumeType {
    OBJECT  = 0,
    BLOCK   = 1,
}

// Added for multi-tenancy
struct Tenant {
    1: i64 id,
    2: string identifier,
}

// Added for multi-tenancy
struct User {
    1: i64 id,
    2: string identifier,
    3: string passwordHash,
    4: string secret,
    5: bool isFdsAdmin,
}

struct VolumeSettings {
   1: required i32 maxObjectSizeInBytes,
   2: required VolumeType volumeType,
   3: required i64 blockDeviceSizeInBytes,
   4: required i64 contCommitlogRetention,
   5: MediaPolicy mediaPolicy,
}

struct VolumeDescriptor {
   1: required string name,
   2: required i64 dateCreated,
   3: required VolumeSettings policy,
   4: required i64 tenantId,
   5: i64 volId,
   6: common.ResourceState state,
}

/**
 * Snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicy {
    1: i64 id;
    /** Name of the policy, should be unique */
    2: string policyName,
    /** Recurrence rule as per iCal format */
    3: string recurrenceRule,
    /** Retention time in seconds */
    4: i64 retentionTimeSeconds,
    5: common.ResourceState state,
    7: i64 timelineTime,
}

struct StreamingRegistrationMsg {
   1:i32 id,
   2:string url,
   6:string http_method,
   3:list<string> volume_names,
   4:i32 sample_freq_seconds,
   5:i32 duration_seconds,
}

service ConfigurationService {
        // // Added for multi-tenancy
    i64 createTenant(1:string identifier)
        throws (1: common.ApiException e),

    list<Tenant> listTenants(1:i32 ignore)
        throws (1: common.ApiException e),

    i64 createUser(1:string identifier, 2: string passwordHash, 3:string secret, 4: bool isFdsAdmin)
        throws (1: common.ApiException e),

    void assignUserToTenant(1:i64 userId, 2:i64 tenantId)
        throws (1: common.ApiException e),

    void revokeUserFromTenant(1:i64 userId, 2:i64 tenantId)
        throws (1: common.ApiException e),

    list<User> allUsers(1:i64 ignore)
        throws (1: common.ApiException e),

    list<User> listUsersForTenant(1:i64 tenantId)
        throws (1: common.ApiException e),

    void updateUser(1: i64 userId, 2:string identifier, 3:string passwordHash, 4:string secret, 5:bool isFdsAdmin)
        throws (1: common.ApiException e),

    // Added for caching
    i64 configurationVersion(1: i64 ignore)
        throws (1: common.ApiException e),

    void createVolume(1:string domainName, 2:string volumeName, 3:VolumeSettings volumeSettings, 4: i64 tenantId)
        throws (1: common.ApiException e),

    i64 getVolumeId(1:string volumeName)
        throws (1: common.ApiException e),

    string getVolumeName(1:i64 volumeId)
        throws (1: common.ApiException e),

    void deleteVolume(1:string domainName, 2:string volumeName)
        throws (1: common.ApiException e),

    VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
        throws (1: common.ApiException e),

    list<VolumeDescriptor> listVolumes(1:string domainName)
        throws (1: common.ApiException e),

    i32 registerStream(
        1:string url,
        2:string http_method,
        3:list<string> volume_names,
        4:i32 sample_freq_seconds,
        5:i32 duration_seconds),

    list<StreamingRegistrationMsg> getStreamRegistrations(1:i32 ignore),
    void deregisterStream(1:i32 registration_id),

    i64 createSnapshotPolicy(1:SnapshotPolicy policy)
        throws (1: common.ApiException e),

    list<SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
        throws (1: common.ApiException e),

    void deleteSnapshotPolicy(1:i64 id)
        throws (1: common.ApiException e),

    void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
        throws (1: common.ApiException e),

    list<SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
        throws (1: common.ApiException e),

    void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
        throws (1: common.ApiException e),

    list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
        throws (1: common.ApiException e),

    void createSnapshot(1:i64 volumeId, 2:string snapshotName, 3:i64 retentionTime, 4:i64 timelineTime)
        throws (1: common.ApiException e),

    list<common.Snapshot> listSnapshots(1:i64 volumeId)
        throws (1: common.ApiException e),

    void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
        throws (1: common.ApiException e),

    // Returns VolumeID of clone
    i64 cloneVolume(1:i64 volumeId, 2:i64 fdsp_PolicyInfoId, 3:string cloneVolumeName, 4:i64 timelineTime)
}
