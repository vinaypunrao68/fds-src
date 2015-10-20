/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "config_types.thrift"

include "common.thrift"
include "node_svc_api.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

/* ------------------------------------------------------------
   Operations on the tenancy, users, volumes and snapshots.
   ------------------------------------------------------------*/

/**
 * Configuration Service
 */
service ConfigurationService {
  /**
   * Create a new Local Domain.
   *
   * @param domainName - A string representing the name of the new Local Domain.
   * @param domainSite - A string representing the site or location of the new Local Domain.
   *
   * @return Returns the Local Domain's ID.
   */
  i64 createLocalDomain(1:string domainName, 2:string domainSite)
      throws (1: common.ApiException e);

  /**
   * Lists currently defined Local Domains.
   *
   * @return Returns the list of Local Domain.
   */
  list<config_types.LocalDomain> listLocalDomains(1:i32 ignore)
      throws (1: common.ApiException e);

  /**
   * Change the name of the given Local Domain.
   *
   * @param oldDomainName - A string representing the old name of the Local Domain.
   * @param newDomainName - A string representing the new name of the Local Domain.
   *
   * @return void.
   */
  void updateLocalDomainName(1:string oldDomainName, 2:string newDomainName)
      throws (1: common.ApiException e);

  /**
   * Change the name of the given Local Domain's site.
   *
   * @param domainName - A string representing the name of the Local Domain whose site is to be changed.
   * @param newSiteName - A string representing the new name of the Local Domain's site.
   *
   * @return void.
   */
  void updateLocalDomainSite(1:string domainName, 2:string newSiteName)
      throws (1: common.ApiException e);

  /**
   * Set the throttle of the given Local Domain.
   *
   * @param domainName - A string representing the name of the Local Domain whose throttle is to be set.
   * @param throttleLevel - A double representing the new throttle level for the Local Domain.
   *
   * @return void.
   */
  void setThrottle(1:string domainName, 2:double throttleLevel)
      throws (1: common.ApiException e);

  /**
   * Set the scavenger actin for the given Local Domain.
   *
   * @param domainName - A string representing the name of the Local Domain whose scavenger action is to be set.
   * @param scavengerAction - A string representing the new scavenger action for the Local Domain. One of
   *                          "enable", "disable", "start", "stop".
   *
   * @return void.
   */
  void setScavenger(1:string domainName, 2:string scavengerAction)
      throws (1: common.ApiException e);

  /**
   * Startup named Local Domain
   *
   * Startup involves starting all processes associated with SM, DM, and AM services.
   *
   * @param domainName - A string representing the name of the Local Domain to be started.
   */
  void startupLocalDomain(1:string domainName)
      throws (1: common.ApiException e);

  /**
   * Shutdown named Local Domain.
   *
   * Shutdown involves stopping all processes associated with SM, DM, and AM services. However,
   * none of them are unregistered from the Domain as would be the case with "removing" services.
   *
   * @param domainName - A string representing the name of the Local Domain to be shutdown.
   *
   * @return void.
   */
  void shutdownLocalDomain(1:string domainName)
      throws (1: common.ApiException e);

  /**
   * Delete currently the named Local Domain.
   *
   * @param domainName - A string representing the name of the Local Domain to be deleted.
   *
   * @return void.
   */
  void deleteLocalDomain(1:string domainName)
      throws (1: common.ApiException e);

  /**
   * Activate currently defined Services for the named Local Domain.
   *
   * If all Service flags are set to False, it will
   * be interpreted to mean activate all Services currently defined for the Node. If there are
   * no Services currently defined for the node, it will be interpreted to mean activate all
   * Services on the Node (SM, DM, and AM), and define all Services for the Node.
   *
   * @param domainName - A string representing the name of the Local Domain whose Services are to be activated.
   * @param sm - A bool indicating whether the SM Service should be activated (True) or not (False)
   * @param dm - A bool indicating whether the DM Service should be activated (True) or not (False)
   * @param am - A bool indicating whether the AM Service should be activated (True) or not (False)
   *
   * @return void.
   */
  void activateLocalDomainServices(1:string domainName, 2:bool sm, 3:bool dm, 4:bool am)
      throws (1: common.ApiException e);

  /**
   * Unlike activateLocalDomainServices, ActivateNode is Node-specific.
   */
  i32 ActivateNode(1:config_types.FDSP_ActivateOneNodeType req),

  /**
   * Lists currently defined Services for the named Local Domain.
   *
   * Both listLocalDomainServices and ListServices do the same thing.
   * ListServices was in place before listLocalDomainServices and so is
   * used by the UI. However, listLocalDomainServices more closely matches
   * our goals in a RESTful API.
   *
   * @return Returns the list of FDSP_Node_Info_Type's.
   */
  list<common.FDSP_Node_Info_Type> listLocalDomainServices(1:string domainName)
      throws (1: common.ApiException e);
  list<common.FDSP_Node_Info_Type> ListServices(1:i32 ignore),

  /**
   * Remove currently defined Services from the named Local Domain.
   *
   * If all Service flags are set to False, it will
   * be interpreted to mean remove all Services currently defined for the Node.
   * Removal means that the Service is unregistered from the Domain and shutdown.
   *
   * @param domainName - A string representing the name of the Local Domain whose Services are to be removed.
   * @param sm - A bool indicating whether the SM Service should be removed (True) or not (False)
   * @param dm - A bool indicating whether the DM Service should be removed (True) or not (False)
   * @param am - A bool indicating whether the AM Service should be removed (True) or not (False)
   *
   * @return void.
   */
  void removeLocalDomainServices(1:string domainName, 2:bool sm, 3:bool dm, 4:bool am)
      throws (1: common.ApiException e);

  /**
   * Unlike removeLocalDomainServices, RemoveServices is Node-specific.
   */
  i32 RemoveServices(1:config_types.FDSP_RemoveServicesType rm_node_req),

  /**
   * Add Service - node specific command
   */
  i32 AddService(1:node_svc_api.NotifyAddServiceMsg req),
  /**
   * Start Service - node specific command
   */
  i32 StartService(1:node_svc_api.NotifyStartServiceMsg req),
   /**
   * Stop Service - node specific command
   */
  i32 StopService(1:node_svc_api.NotifyStopServiceMsg req),
   /**
   * Remove Service - node specific command
   */
  i32 RemoveService(1:node_svc_api.NotifyRemoveServiceMsg req),

  /**
   * Create a new tenant.
   *
   * @param identifier a string representing the tenants identifier or name
   *
   * @return Returns the tenants id
   */
  i64 createTenant(1:string identifier)
      throws (1: common.ApiException e);

  /**
   * Enumerate tenants.
   *
   * @retruns Returns a list of tenants
   */
  list<config_types.Tenant> listTenants(1:i32 ignore)
      throws (1: common.ApiException e);

  /**
   * Create a user.
   *
   * @param identifier a string representing the user identifier, i.e. name
   * @param passwordHash a string representing the users password hash
   * @param secret a string represetning the secret passphrase
   * @param isFdsAdmin a boolean flag indicating if the user has adminstration permissions
   *
   * @return Returns the users id
   */
  i64 createUser(1:string identifier, 2: string passwordHash, 3:string secret, 4: bool isFdsAdmin)
      throws (1: common.ApiException e),

  /**
   * Assign user to a tenant.
   *
   * @param userId the user's uuid
   * @param tenantId the tenant's uuid
   */
  void assignUserToTenant(1:i64 userId, 2:i64 tenantId)
      throws (1: common.ApiException e),

  /**
   * Revoke user from tenant.
   *
   * @param userId the user's uuid
   * @param tenantId the tenant's uuid
   */
  void revokeUserFromTenant(1:i64 userId, 2:i64 tenantId)
      throws (1: common.ApiException e),

  /**
   * Enumerate users.
   *
   * @return Returns a list of users
   */
  list<config_types.User> allUsers(1:i64 ignore)
      throws (1: common.ApiException e),

  /**
   * Enumerate tenant's user list.
   *
   * @param tenantId the tenant's uuid
   *
   * @return Returns
   */
  list<config_types.User> listUsersForTenant(1:i64 tenantId)
      throws (1: common.ApiException e),

  /**
   * Modify a user.
   *
   * @param userId the user's uuid
   * @param identifier a string representing the user identifier, i.e. name
   * @param passwordHash a string representing the users password hash
   * @param secret a string represetning the secret passphrase
   * @param isFdsAdmin a boolean flag indicating if the user has adminstration permissions
   */
  void updateUser(
    1: i64 userId,
    2:string identifier,
    3:string passwordHash,
    4:string secret,
    5:bool isFdsAdmin)
      throws (1: common.ApiException e),

  /**
   * Configuration Version
   *
   * @return Returns the version of the configuration key-value store
   */
  i64 configurationVersion(1: i64 ignore)
      throws (1: common.ApiException e),

  /**
   * Create a volume.
   *
   * @param domainName a string representing the local domain name
   * @param volumeName a string representing the volume name
   * @param volumeSettings a VolumeSettings object
   * @param tenantId the tenant's uuid
   */
  void createVolume(
    1:string domainName,
    2:string volumeName,
    3:config_types.VolumeSettings volumeSettings,
    4: i64 tenantId)
      throws (1: common.ApiException e),

# TODO Returns the volume id

  /**
   * Retrieve FDS volume identifier.
   *
   * @param volumeName a string representing the volume name
   *
   * @return the volume uuid
   */
  i64 getVolumeId(1:string volumeName)
      throws (1: common.ApiException e),

  /**
   * Retrieve FDS volume name.
   *
   * @param volumeId the volume's uuid
   *
   * @return Returns a string representing the volume name
   */
  string getVolumeName(1:i64 volumeId)
      throws (1: common.ApiException e),

  common.FDSP_VolumeDescType GetVolInfo(1:config_types.FDSP_GetVolInfoReqType vol_info_req)
      throws (1:config_types.FDSP_VolumeNotFound not_found),

  /**
   * Modify volume attributes. Actually used to create and clone a
   * volume as well.
   *
   * @param mod_vol_req volume attributes.
   *
   * @return Zero for success, non-zero otherwise.
   */
  i32 ModifyVol(1:config_types.FDSP_ModifyVolType mod_vol_req),

  /**
   * Delete FDS volume.
   *
   * @param domainName a string representing the local domain name
   * @param volumeName a string representing the volume name
   */
  void deleteVolume(1:string domainName, 2:string volumeName)
      throws (1: common.ApiException e),

# TODO Delete volume by id

  /**
   * Request volume status information.
   *
   * @param domainName a string representing the local domain name
   * @param volumeName a string representing the volume name
   */
  config_types.VolumeDescriptor statVolume(
    1:string domainName,
    2:string volumeName)
      throws (1: common.ApiException e),

  # TODO add a statVolume by volume id

  /**
   * Enumerate volumes.
   *
   * Both listVolumes and ListVolumes do the same thing, list
   * volumes and their attributes within the default domain, although
   * ListVolumes provides more attributes than listVolumes.
   * ListVolumes was in place before listVolumes and so is
   * used by the UI. However, listVolumes more closely matches
   * our goals in a RESTful API, but with less volume attributes.
   *
   * @param domainName a string representing the local domain name
   */
  list<config_types.VolumeDescriptor> listVolumes(1:string domainName)
      throws (1: common.ApiException e),
  list <common.FDSP_VolumeDescType> ListVolumes(1:i32 ignore),

  /**
   * Register a statistic stream.
   *
   * @param url a string represening the URL
   * @param http_method the HTTP method
   * @param volume_names a list of strings representing the associated volume names
   * @param sample_freq_seconds the sample freqency. in seconds
   * @param duration_seconds the duration, in seocnds
   *
   * @return Returns statistics stream uuid
   */
  i32 registerStream(
      1:string url,
      2:string http_method,
      3:list<string> volume_names,
      4:i32 sample_freq_seconds,
      5:i32 duration_seconds),

  /**
   * Enumerate stream registrations.
   *
   * @return Returns a list of StreamRegisterationMsg objects
   */
  list<config_types.StreamingRegistrationMsg> getStreamRegistrations(1:i32 ignore),

  /**
   * Unregister a statistics stream.
   *
   * @param regsiteration_id tghe statictics stream uuid
   */
  void deregisterStream(1:i32 registration_id),

  /**
   * Create a new snapshot policy.
   *
   * @param policy the SnapshotPolicy object
   *
   * @return Returns snapshot policy id
   */
  i64 createSnapshotPolicy(1:config_types.SnapshotPolicy policy)
      throws (1: common.ApiException e),

  /**
   * Enumerate snapshot policies.
   *
   * @return Returns a list of SnapshotPolicy objects
   */
  list<config_types.SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
      throws (1: common.ApiException e),

  /**
   * Delete a snapshot policy.
   *
   * @param id the snapshot id to be deleted
   */
  void deleteSnapshotPolicy(1:i64 id)
      throws (1: common.ApiException e),

  /**
   * Set a snapshot policy on a volume.
   *
   * @param volumeId the volume uuid
   * @param policyId the policy uuid
   */
  void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
      throws (1: common.ApiException e),

  /**
   * Enumerate a volume's snapshot policies.
   *
   * @param volumeId the volume uuid
   *
   * @return Returns a list of SnapshotPolicy objects
   */
  list<config_types.SnapshotPolicy> listSnapshotPoliciesForVolume(
    1:i64 volumeId)
      throws (1: common.ApiException e),

  /**
   * Unset a snapshot policy from a volume.
   *
   * @param volumeId the volume uuid
   * @param policyId the policy uuid
   */
  void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
      throws (1: common.ApiException e),

  /**
   * Enumate volumes using snapshot policy.
   *
   * @param policyId the policy uuid
   *
   * @return Returns list of volumeId
   */
  list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
      throws (1: common.ApiException e),

  /**
   * Create's a snapshot.
   *
   * @param volumeId the volume uuid
   * @param snapshotName a string representing the snapshot name
   * @param retentionTime the retention time, in seconds
   * @param timelineTime the timeline time, in seconds
   */
  void createSnapshot(
    1:i64 volumeId,
    2:string snapshotName,
    3:i64 retentionTime,
    4:i64 timelineTime)
      throws (1: common.ApiException e),

  /**
   * Delete the snapshot with the given snapshotId
   */
  void deleteSnapshot(1:i64 volumeId, 2:i64 snapshotId)
      throws (1: common.ApiException e),

  /**
   * Enumerate snapshots.
   *
   * @param volumeId the volume id
   *
   * @return Returns a list of snapshots
   */
  list<common.Snapshot> listSnapshots(1:i64 volumeId)
      throws (1: common.ApiException e),

  /**
   * Reset a volume to a previous snapshot.
   *
   * @param volumeId the volume uuid
   * @param snapshotId the snapshot uuid
   */
  void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
      throws (1: common.ApiException e),

  /**
   * Retrieve volume clone identifier.
   *
   * @param volumeId the volume uuid
   * @param fdspPolicyInfoId the policy infomration uuid
   * @param cloneVolumeName the clone volume name
   * @param timelimeTimes the timeline time, in seconds
   *
   * @return Returns the clone volume id
   */
  i64 cloneVolume(
    1:i64 volumeId,
    2:i64 fdsp_PolicyInfoId,
    3:string cloneVolumeName,
    4:i64 timelineTime)

  /**
   * Create a new QoS policy.
   *
   * @param policy_name - A string representing the name of the policy. Must be unique within a Global Domain.
   * @param iops_min - An i64 representing the minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
   *                   "Service Level Agreement".
   * @param iops_max - An i64 representing the maximum IOPS guaranteed by the policy. Also referred to as "Limit".
   * @param rel_prio - An i32 representing the relative priority of requests against Volumes with this policy compared
   *                   to requests against Volumes with different relative priorities.
   *
   * @return Returns common.FDSP_PolicyInfoType
   */
  common.FDSP_PolicyInfoType createQoSPolicy(1:string policy_name, 2:i64 iops_min, 3:i64 iops_max, 4:i32 rel_prio)
      throws (1: common.ApiException e),

  /**
   * Enumerate QoS policies.
   *
   * @return Returns a list of FDSP_PolicyInfoType objects
   */
  list<common.FDSP_PolicyInfoType> listQoSPolicies(1:i64 unused)
      throws (1: common.ApiException e),

  /**
   * Modify a QoS policy.
   *
   * @param current_policy_name - A string representing the current name of the policy.
   * @param new_policy_name - A string representing the new name of the policy. Must be unique within a Global Domain.
   *                          May be the same as current_policy_name if the name is not changing.
   * @param iops_min - An i64 representing the new minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
   *                   "Service Level Agreement".
   * @param iops_max - An i64 representing the new maximum IOPS guaranteed by the policy. Also referred to as "Limit".
   * @param rel_prio - An i32 representing the new relative priority of requests against Volumes with this policy compared
   *                   to requests against Volumes with different relative priorities.
   *
   * @return Returns common.FDSP_PolicyInfoType containing the modified QoS Policy.
   */
  common.FDSP_PolicyInfoType modifyQoSPolicy(1:string current_policy_name, 2:string new_policy_name, 3:i64 iops_min,
                                             4:i64 iops_max, 5:i32 rel_prio)
      throws (1: common.ApiException e),

  /**
   * Delete a QoS policy.
   *
   * @param policy_name the name of the QoS policy to be deleted
   */
  void deleteQoSPolicy(1:string policy_name)
      throws (1: common.ApiException e),

  /* Subscription Management */

  /**
   * These APIs are only valid within the context of the Master Domain.
   */

  /**
   * Create a subscription.
   *
   * @param subName: A string. The name of the subscription.
   * @param tenantID: An i64. The ID of the Tenant owning the subscription.
   * @param primaryDomainID: An i32. The ID of the local domain where the primary volume is located.
   * @param primaryVolumeID: An i64. The ID of the primary volume within the context of the primary domain.
   * @param replicaDomainID: An i32. The ID of the local domain where the replica volume is to be located.
   * @param subType: An i32. One of the config_types.SubscriptionType enumerated values indicating the type of
   *    subscription.
   * @param schedType: An i32. One of the config_types.SubscriptionScheduleType enumerated values indicating the type
   *    of refresh schedule.
   * @param intervalSize: An i64. The interval quantity associated with schedType.
   *
   * @return i64: ID of the created subscription.
   */
  i64 createSubscription(
        1: string   subName,
        2: i64      tenantID,
        3: i32      primaryDomainID,
        4: i64      primaryVolumeID,
        5: i32      replicaDomainID,
        6: config_types.SubscriptionType subType,
        7: config_types.SubscriptionScheduleType schedType,
        8: i64      intervalSize)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List subscriptions.
   *
   * Several different ways to list subscriptions thought to be useful
   * either externally or internally are provided.
   */

  /**
   * List all subscriptions defined in the global domain.
   *
   * @param ignore: An i32. Thrift has difficulty with empty formal parameter lists.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list <config_types.SubscriptionDescriptor> listSubscriptionsAll(1: i32 ignore)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions defined in the global domain for the given tenant.
   *
   * @param tenantID: An i64. The ID of the Tenant whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list <config_types.SubscriptionDescriptor> listTenantSubscriptionsAll(1: i64 tenantID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given primary domain.
   *
   * @param primaryDomainID: An i32. The ID of the primary domain whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listSubscriptionsPrimaryDomain(1: i32 primaryDomainID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given primary domain and tenant.
   *
   * @param primaryDomainID: An i32. The ID of the primary domain whose subscriptions are to be listed.
   * @param tenantID: An i64. The ID of the Tenant whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listTenantSubscriptionsPrimaryDomain(1: i32 primaryDomainID, 2: i64 tenantID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given replica domain.
   *
   * @param replicaDomainID: An i32. The ID of the replica domain whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listSubscriptionsReplicaDomain(1: i32 replicaDomainID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given replica domain and tenant.
   *
   * @param replicaDomainID: An i32. The ID of the replica domain whose subscriptions are to be listed.
   * @param tenantID: An i64. The ID of the Tenant whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listTenantSubscriptionsReplicaDomain(1: i32 replicaDomainID, 2: i64 tenantID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given primary volume.
   *
   * @param primaryVolumeID: An i64. The ID of the primary volume whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listSubscriptionsPrimaryVolume(1: i64 primaryVolumeID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * List all subscriptions for the given replica volume.
   *
   * @param replicaVolumeID: An i64. The ID of the replica volume whose subscriptions are to be listed.
   *
   * @return list<config_types.SubscriptionDescriptor>: The requested subscriptions.
   */
  list<config_types.SubscriptionDescriptor> listSubscriptionsReplicaVolume(1: i64 replicaVolumeID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain);

  /**
   * Retrieve subscription information from subscription name.
   *
   * @param subName: A string. The name of the subscription whose details are to be returned.
   * @param tenantID: An i64. The ID of the Tenant whose subscription is named. (Multiple tenants may have like
   *                  named subscriptions.)
   *
   * @return config_types.SubscriptionDescriptor: The details of the named subscription.
   */
  config_types.SubscriptionDescriptor getSubscriptionInfoName(1: string subName, 2: i64 tenantID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain,
            3: config_types.SubscriptionNotFound notFound);

  /**
   * Retrieve subscription information from subscription ID.
   *
   * @param subID: An i64. The numeric identifier of the subscription whose details are to be returned.
   *
   * @return config_types.SubscriptionDescriptor: The details of the named subscription.
   */
  config_types.SubscriptionDescriptor getSubscriptionInfoID(1: i64 subID)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain,
            3: config_types.SubscriptionNotFound notFound);

  /**
   * Update subscription attributes. The following are *not* modifiable:
   * id, tenantID, primaryDomainID, primaryVolumeID, replicaDomainID, createTime
   *
   * @param subMods all subscription attributes with modifications applied. "id" must be supplied. Changes to the
   *    following are rejected: tenantID, primaryDomainID, primaryVolumeID, replicaDomainID, createTime
   *
   * @return void.
   */
  void updateSubscription(1: config_types.SubscriptionDescriptor subMods)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain,
            3: config_types.SubscriptionNotFound notFound,
            4: config_types.SubscriptionNotModified notModified);

  /**
   * Delete a subscription identified by subscription name.
   *
   * @param subName: An string. The name of the subscription to be deleted.
   * @param tenantID: An i64. The ID of the Tenant whose subscription is named. (Multiple tenants may have like
   *                  named subscriptions.)
   * @param dematerialize: A bool. "true" if the replica volume contents resulting from the subscription are to be
   *    deleted. If setting "true" results in all replica volume contents being deleted, the volume will be deleted
   *    as well.
   *
   * @return void
   */
  void deleteSubscriptionName(1: string subName, 2: i64 tenantID, 3: bool dematerialize)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain,
            3: config_types.SubscriptionNotFound notFound,
            4: config_types.SubscriptionNotModified notModified);

  /**
   * Delete a subscription identified by subscription ID.
   *
   * @param subID: An i64. The numeric identifier of the subscription to be deleted.
   * @param dematerialize: A bool. "true" if the replica volume contents resulting from the subscription are to be
   *    deleted. If setting "true" results in all replica volume contents being deleted, the volume will be deleted as
   *    well.
   *
   * @return void
   */
  void deleteSubscriptionID(1: i64 subID, 2: bool dematerialize)
    throws (1: common.ApiException e,
            2: common.NotMasterDomain notMasterDomain,
            3: config_types.SubscriptionNotFound notFound,
            4: config_types.SubscriptionNotModified notModified);

}
