/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

include "config_types.thrift"

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

/* ------------------------------------------------------------
   Operations on the tenancy, users, volumes and snapshots.
   ------------------------------------------------------------*/

/**
 * Configuration Service
 */
service ConfigurationService {
  /** Create a Local Domain. */
  i64 createLocalDomain(1:string domainName, 2:i64 domainId)
      throws (1: common.ApiException e);
  /** Create a new tenant. */
  i64 createTenant(1:string identifier)
      throws (1: common.ApiException e);
  /** Enumerate tenants. */
  list<config_types.Tenant> listTenants(1:i32 ignore)
      throws (1: common.ApiException e);
  /** Create a user. */
  i64 createUser(1:string identifier, 2: string passwordHash, 3:string secret, 4: bool isFdsAdmin)
      throws (1: common.ApiException e),
  /** Assign user to a tenant. */
  void assignUserToTenant(1:i64 userId, 2:i64 tenantId)
      throws (1: common.ApiException e),
  /** Revoke user from tenant. */
  void revokeUserFromTenant(1:i64 userId, 2:i64 tenantId)
      throws (1: common.ApiException e),
  /** Enumerate users. */
  list<config_types.User> allUsers(1:i64 ignore)
      throws (1: common.ApiException e),
  /** Enumerate tenant's user list. */
  list<config_types.User> listUsersForTenant(1:i64 tenantId)
      throws (1: common.ApiException e),
  /** Modify a user. */
  void updateUser(1: i64 userId, 2:string identifier, 3:string passwordHash, 4:string secret, 5:bool isFdsAdmin)
      throws (1: common.ApiException e),
  /** TODO ? */
  i64 configurationVersion(1: i64 ignore)
      throws (1: common.ApiException e),
  /** Create a FDS volume. */
  void createVolume(1:string domainName, 2:string volumeName, 3:config_types.VolumeSettings volumeSettings, 4: i64 tenantId)
      throws (1: common.ApiException e),
  /** Retrieve FDS volume identifier. */
  i64 getVolumeId(1:string volumeName)
      throws (1: common.ApiException e),
  /** Retrieve FDS volume name. */
  string getVolumeName(1:i64 volumeId)
      throws (1: common.ApiException e),
  /** Delete FDS volume. */
  void deleteVolume(1:string domainName, 2:string volumeName)
      throws (1: common.ApiException e),
  /** Request volume status information. */
  config_types.VolumeDescriptor statVolume(1:string domainName, 2:string volumeName)
      throws (1: common.ApiException e),
  /** Enumerate volumes. */
  list<config_types.VolumeDescriptor> listVolumes(1:string domainName)
      throws (1: common.ApiException e),
  /** Register a stat stream. */
  i32 registerStream(
      1:string url,
      2:string http_method,
      3:list<string> volume_names,
      4:i32 sample_freq_seconds,
      5:i32 duration_seconds),
  /** Enumerate stream registrations. */
  list<config_types.StreamingRegistrationMsg> getStreamRegistrations(1:i32 ignore),
  /** Unregister a stat stream. */
  void deregisterStream(1:i32 registration_id),
  /** Create a new snapshot policy. */
  i64 createSnapshotPolicy(1:config_types.SnapshotPolicy policy)
      throws (1: common.ApiException e),
  /** Enumerate snapshot policies. */
  list<config_types.SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
      throws (1: common.ApiException e),
  /** Delete a snapshot policy. */
  void deleteSnapshotPolicy(1:i64 id)
      throws (1: common.ApiException e),
  /** Set a snapshot policy on a volume. */
  void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
      throws (1: common.ApiException e),
  /** Enumerate a volume's snapshot policies. */
  list<config_types.SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
      throws (1: common.ApiException e),
  /** Unset a snapshot policy from a volume. */
  void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
      throws (1: common.ApiException e),
  /** Enumate volumes using snapshot policy. */
  list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
      throws (1: common.ApiException e),
  /** Initiate a snapshot. */
  void createSnapshot(1:i64 volumeId, 2:string snapshotName, 3:i64 retentionTime, 4:i64 timelineTime)
      throws (1: common.ApiException e),
  /** Enumerate snapshots. */
  list<common.Snapshot> listSnapshots(1:i64 volumeId)
      throws (1: common.ApiException e),
  /** Reset a volume to a previous snapshot. */
  void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
      throws (1: common.ApiException e),
  /** Retrieve volume clone identifier. */
  i64 cloneVolume(1:i64 volumeId, 2:i64 fdsp_PolicyInfoId, 3:string cloneVolumeName, 4:i64 timelineTime)
}
