/**
 * Copyright (c) 2015 Formation Data Systems. All rights reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.s3.S3Endpoint;
import com.google.common.collect.Lists;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

// TODO: This still goes direct to the Native C++ OM thrift server (assuming the Thrift client connection
// to ConfigurationService.Iface hasn't changed).  The XDI class has changed to use the
// OMConfigServiceRestClientImpl, which communicates with the Java OM. Should this be modified to
// use the OM rest client for those operations it supports?  Is this class even necessary anymore?  It is
// only used by FdsAuthenticator/FdsAuthorizer
// TODO: authorize here
public class XdiConfigurationApi implements ConfigurationApi {
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
        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY);
        config.createVolume(S3Endpoint.FDS_S3, systemFolderName(tenantId), volumeSettings, tenantId);
        dropCache();
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
        return userId;
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId)
            throws ApiException, TException {
        config.assignUserToTenant(userId, tenantId);
        dropCache();
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId)
            throws ApiException, TException {
        config.revokeUserFromTenant(userId, tenantId);
        dropCache();
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
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void attachSnapshotPolicy(long volumeId, long policyId)
            throws ApiException, org.apache.thrift.TException {
        config.attachSnapshotPolicy(volumeId, policyId);
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
    }

    @Override
    public long cloneVolume(long volumeId, long fdsp_PolicyInfoId, String clonedVolumeName, long timelineTime)
            throws org.apache.thrift.TException {
        long clonedVolumeId = config.cloneVolume(volumeId, fdsp_PolicyInfoId, clonedVolumeName, timelineTime);
        if (clonedVolumeId <= 0) {
            clonedVolumeId = config.getVolumeId(clonedVolumeName);
        }
        dropCache();

        return clonedVolumeId;
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
