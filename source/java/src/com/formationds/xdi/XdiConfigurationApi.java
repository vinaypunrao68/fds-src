/**
 * Copyright (c) 2015 Formation Data Systems. All rights reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.commons.events.*;
import com.formationds.om.events.EventManager;
import com.formationds.security.AuthenticationToken;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.s3.S3Endpoint;
import com.google.common.collect.Lists;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Supplier;

// TODO: authorize here
public class XdiConfigurationApi implements ConfigurationApi, ConfigurationService.Iface, Supplier<CachedConfiguration> {
    private static final Logger LOG = Logger.getLogger(XdiConfigurationApi.class);
    private static final long   KEY = 0;
    private final ConfigurationService.Iface                   config;
    private final ConcurrentHashMap<Long, CachedConfiguration> map;

    public XdiConfigurationApi(ConfigurationService.Iface config) throws Exception {
        this.config = config;
        map = new ConcurrentHashMap<>();
        map.compute(KEY, (k, v) -> {
            try {
                return new CachedConfiguration(config);
            } catch (Exception e) {
                LOG.error("Unable to load configuration", e);
                throw new RuntimeException(e);
            }
        });
        new Thread(new Updater()).start();
    }

    public static String systemFolderName(long tenantId) {
        return "SYSTEM_VOLUME_" + tenantId;
    }

    enum ConfigEvent implements EventDescriptor {

        CREATE_TENANT(EventCategory.VOLUMES, "Created tenant {0}", "identifier"),
        CREATE_USER(EventCategory.SYSTEM, "Created user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
      UPDATE_USER(EventCategory.SYSTEM, "Updated user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
      ASSIGN_USER_TENANT(EventCategory.SYSTEM, "Assigned user {0} to tenant {1}", "userId", "tenantId"),
      REVOKE_USER_TENANT(EventCategory.SYSTEM, "Revoked user {0} from tenant {1}", "userId", "tenantId"),
      CREATE_VOLUME(EventCategory.VOLUMES, "Created volume: domain={0}; name={1}; tenantId={2}; type={3}; size={4}",
                    "domainName", "volumeName", "tenantId", "volumeType", "maxSize" ),
      DELETE_VOLUME(EventCategory.VOLUMES, "Deleted volume: domain={0}; name={1}", "domainName", "volumeName"),
      CREATE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Created snapshot policy {0} {1} {2}; id={3}",
                             "name", "recurrence", "retention", "id" ),
      DELETE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Deleted snapshot policy: id={0}", "id"),
      ATTACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Attached snapshot policy {0} to volume id {1}",
                             "policyId", "volumeId"),
      DETACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Detached snapshot policy {0} from volume id {1}",
                             "policyId", "volumeId"),
      CREATE_SNAPSHOT(EventCategory.VOLUMES, "Created snapshot {0} of volume {1}.  Retention time = {2}",
                      "snapshotName", "volumeId", "retentionTime"),

      CLONE_VOLUME(EventCategory.VOLUMES, "Cloned volume {0} with policy id {1}; clone name={2}; Cloned volume Id = {3}",
                   "volumeId", "policyId", "clonedVolumeName", "clonedVolumeId" ),
      RESTORE_CLONE(EventCategory.VOLUMES, "Restored cloned volume {0} with snapshot id {1}", "volumeId", "snapshotId");

      private final EventType type;
      private final EventCategory category;
      private final EventSeverity severity;
      private final String defaultMessage;
      private final List<String> argNames;

      private ConfigEvent(EventCategory category, String defaultMessage, String... argNames) {
          this(EventType.USER_ACTIVITY, category, EventSeverity.CONFIG, defaultMessage, argNames);
      }
      private ConfigEvent(EventType type, EventCategory category, EventSeverity severity,
                          String defaultMessage, String... argNames) {
          this.type = type;
          this.category = category;
          this.severity = severity;
          this.defaultMessage = defaultMessage;
          this.argNames = (argNames != null ? Arrays.asList(argNames) : new ArrayList<String>(0));
      }

      public String key()               { return name(); }
      public List<String> argNames()    { return argNames; }
      public EventType type()           { return type;  }
      public EventCategory category()   { return category; }
      public EventSeverity severity()   { return severity; }
      public String defaultMessage()    { return defaultMessage; }

  }

    public long createSnapshotPolicy(final String name, final String recurrence, final long retention, final long timelineTime)
            throws TException {
        final SnapshotPolicy apisPolicy = new SnapshotPolicy();

        apisPolicy.setPolicyName(name);
        apisPolicy.setRecurrenceRule(recurrence);
        apisPolicy.setRetentionTimeSeconds(retention);
        apisPolicy.setTimelineTime(timelineTime);

        return createSnapshotPolicy(apisPolicy);
    }

    @Override
    public Collection<User> listUsers() {
        return fillCacheMaybe().users();
    }

    @Override
    public Optional<Tenant> tenantFor(long userId) {
        return fillCacheMaybe().tenantFor(userId);
    }

    @Override
    public Long tenantId(long userId) throws SecurityException {
        return fillCacheMaybe().tenantId(userId);
    }

    @Override
    public User getUser(long userId) {
        return fillCacheMaybe().usersById()
                               .get(userId);
    }

    @Override
    public User getUser(String login) {
        return fillCacheMaybe().usersByName().get(login);
    }

    @Override
    public long createTenant(String identifier)
            throws ApiException, TException {
        long tenantId = config.createTenant(identifier);
        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0, 0);
        config.createVolume(S3Endpoint.FDS_S3, systemFolderName(tenantId), volumeSettings, tenantId);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.CREATE_TENANT, identifier);
        return tenantId;
    }

    @Override
    public List<Tenant> listTenants(int ignore)
            throws ApiException, TException {
        return fillCacheMaybe().tenants();
    }

    @Override
    public long createUser(String identifier, String passwordHash, String secret, boolean isFdsAdmin)
            throws ApiException, TException {
        long userId = config.createUser(identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.CREATE_USER, identifier, isFdsAdmin, userId);
        return userId;
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId)
            throws ApiException, TException {
        config.assignUserToTenant(userId, tenantId);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.ASSIGN_USER_TENANT, userId, tenantId);
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId)
            throws ApiException, TException {
        config.revokeUserFromTenant(userId, tenantId);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.REVOKE_USER_TENANT, userId, tenantId);
    }

    @Override
    public List<User> allUsers(long ignore)
            throws ApiException, TException {
        CachedConfiguration cached = fillCacheMaybe();
        return Lists.newArrayList(cached.users());
    }


    @Override
    public List<User> listUsersForTenant(long tenantId)
            throws ApiException, TException {
        return fillCacheMaybe().listUsersForTenant(tenantId);
    }

    @Override
    public void updateUser(long userId, String identifier, String passwordHash, String secret, boolean isFdsAdmin)
            throws ApiException, TException {
        config.updateUser(userId, identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.UPDATE_USER, identifier, isFdsAdmin, userId);
    }

    @Override
    public long configurationVersion(long ignore)
            throws ApiException, TException {
        return fillCacheMaybe().getVersion();
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumeSettings volumeSettings, long tenantId)
            throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumeSettings, tenantId);
        dropCache();

        VolumeType vt = volumeSettings.getVolumeType();
        long maxSize = (VolumeType.BLOCK.equals(vt) ?
                                volumeSettings.getBlockDeviceSizeInBytes() :
                                volumeSettings.getMaxObjectSizeInBytes());
        EventManager.notifyEvent(ConfigEvent.CREATE_VOLUME, domainName, volumeName, tenantId,
                                 vt.name(),
                                 maxSize);
    }

    @Override
    public long getVolumeId(String volumeName)
            throws ApiException, org.apache.thrift.TException {
        return config.getVolumeId(volumeName);
    }

    @Override
    public String getVolumeName(long volumeId)
            throws ApiException, org.apache.thrift.TException {
        return config.getVolumeName(volumeId);
    }

    @Override
    public void deleteVolume(String domainName, String volumeName)
            throws ApiException, TException {
        config.deleteVolume(domainName, volumeName);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.DELETE_VOLUME, domainName, volumeName);
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName)
            throws ApiException, TException {
        return fillCacheMaybe().volumesByName()
                .get(volumeName);
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName)
            throws ApiException, TException {
        return Lists.newArrayList(fillCacheMaybe().volumesByName()
                .values());
    }

    @Override
    public int registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds)
            throws TException {
        return config.registerStream(url, http_method, volume_names, sample_freq_seconds, duration_seconds);
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations(int ignore)
            throws TException {
        return config.getStreamRegistrations(ignore);
    }

    @Override
    public void deregisterStream(int registration_id)
            throws TException {
        config.deregisterStream(registration_id);
    }

    @Override
    public long createSnapshotPolicy(com.formationds.apis.SnapshotPolicy policy)
            throws ApiException, org.apache.thrift.TException {
        long l = config.createSnapshotPolicy(policy);
        // TODO: is the value returned the new policy id?
        EventManager.notifyEvent(ConfigEvent.CREATE_SNAPSHOT_POLICY, policy.getPolicyName(), policy.getRecurrenceRule(),
                                 policy.getRetentionTimeSeconds(), l);
        return l;
    }

    @Override
    public List<com.formationds.apis.SnapshotPolicy> listSnapshotPolicies(long unused)
            throws ApiException, org.apache.thrift.TException {
        return config.listSnapshotPolicies(unused);
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void deleteSnapshotPolicy(long id)
            throws ApiException, org.apache.thrift.TException {
        config.deleteSnapshotPolicy(id);
        EventManager.notifyEvent(ConfigEvent.DELETE_SNAPSHOT_POLICY, id);
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void attachSnapshotPolicy(long volumeId, long policyId)
            throws ApiException, org.apache.thrift.TException {
        config.attachSnapshotPolicy(volumeId, policyId);
        EventManager.notifyEvent(ConfigEvent.ATTACH_SNAPSHOT_POLICY, policyId, volumeId);
    }

    @Override
    public List<com.formationds.apis.SnapshotPolicy> listSnapshotPoliciesForVolume(long volumeId)
            throws ApiException, org.apache.thrift.TException {
        return config.listSnapshotPoliciesForVolume(volumeId);
    }

    @Override
    public void detachSnapshotPolicy(long volumeId, long policyId)
            throws ApiException, org.apache.thrift.TException {
        config.detachSnapshotPolicy(volumeId, policyId);
        EventManager.notifyEvent(ConfigEvent.DETACH_SNAPSHOT_POLICY, policyId, volumeId);
    }

    @Override
    public List<Long> listVolumesForSnapshotPolicy(long policyId)
            throws ApiException, org.apache.thrift.TException {
        return config.listVolumesForSnapshotPolicy(policyId);
    }

    @Override
    public void createSnapshot(long volumeId, String snapshotName, long retentionTime, long timelineTime)
            throws ApiException, TException {
        config.createSnapshot(volumeId, snapshotName, retentionTime, timelineTime);
        // TODO: is there a generated snapshot id?
        EventManager.notifyEvent(ConfigEvent.CREATE_SNAPSHOT, snapshotName, volumeId, retentionTime);
    }

    @Override
    public List<com.formationds.apis.Snapshot> listSnapshots(long volumeId)
            throws ApiException, org.apache.thrift.TException {
        return config.listSnapshots(volumeId);
    }

    @Override
    public void restoreClone(long volumeId, long snapshotId)
            throws ApiException, org.apache.thrift.TException {
        config.restoreClone(volumeId, snapshotId);
        EventManager.notifyEvent(ConfigEvent.RESTORE_CLONE, volumeId, snapshotId);
    }

    @Override
    public long cloneVolume(long volumeId, long fdsp_PolicyInfoId, String clonedVolumeName, long timelineTime)
            throws org.apache.thrift.TException {
        long clonedVolumeId = config.cloneVolume(volumeId, fdsp_PolicyInfoId, clonedVolumeName, timelineTime);
        if (clonedVolumeId <= 0) {
            clonedVolumeId = config.getVolumeId(clonedVolumeName);
        }
        dropCache();

        EventManager.notifyEvent(ConfigEvent.CLONE_VOLUME, volumeId, fdsp_PolicyInfoId, clonedVolumeName, clonedVolumeId);
        return clonedVolumeId;
    }

    @Override
    public CachedConfiguration get() {
        return fillCacheMaybe();
    }


    private void dropCache() {
        map.clear();
    }

    private CachedConfiguration fillCacheMaybe() {
        return map.computeIfAbsent(KEY, k -> {
            try {
                return new CachedConfiguration(config);
            } catch (Exception e) {
                LOG.error("Error refreshing configuration", e);
                throw new RuntimeException(e);
            }
        });
    }


    private class Updater
            implements Runnable {
        @Override
        public void run() {
            while (true) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                }
                map.compute(KEY, (k, v) -> {
                    try {
                        return obtainConfig(v);
                    } catch (Exception e) {
                        LOG.error("Error refreshing cache", e);
                        return v;
                    }
                });
            }
        }

        private CachedConfiguration obtainConfig(CachedConfiguration v)
                throws Exception {
            if (v == null) {
                return new CachedConfiguration(config);
            } else {
                long currentVersion = config.configurationVersion(0);
                if (currentVersion != v.getVersion()) {
                    LOG.debug("Cache updated, refreshing");
                    return new CachedConfiguration(config);
                } else {
                    return v;
                }
            }
        }
    }
}
