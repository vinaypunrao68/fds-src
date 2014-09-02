package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.google.common.collect.Lists;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Supplier;

public class ConfigurationServiceCache implements ConfigurationService.Iface, Supplier<CachedConfiguration> {
    private static final Logger LOG = Logger.getLogger(ConfigurationServiceCache.class);
    private static final long KEY = 0;
    private final ConfigurationService.Iface config;
    private final ConcurrentHashMap<Long, CachedConfiguration> map;

    public ConfigurationServiceCache(ConfigurationService.Iface config) throws Exception {
        this.config = config;
        map = new ConcurrentHashMap<>();
        map.compute(KEY, (k, v) -> {
            try {
                return new CachedConfiguration(config);
            } catch (Exception e) {
                LOG.error("Unable to load configuration", e);
                throw new RuntimeException(e);
            }
Co        });
        new Thread(new Updater()).start();
    }

    @Override
    public long createTenant(String identifier) throws ApiException, TException {
        long tenantId = config.createTenant(identifier);
        dropCache();
        return tenantId;
    }


    @Override
    public List<Tenant> listTenants(int ignore) throws ApiException, TException {
        return fillCacheMaybe().tenants();
    }

    @Override
    public long createUser(String identifier, String passwordHash, String secret, boolean isFdsAdmin) throws ApiException, TException {
        long userId = config.createUser(identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
        return userId;
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId) throws ApiException, TException {
        config.assignUserToTenant(userId, tenantId);
        dropCache();
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId) throws ApiException, TException {
        config.revokeUserFromTenant(userId, tenantId);
        dropCache();
    }

    @Override
    public List<User> allUsers(long ignore) throws ApiException, TException {
        CachedConfiguration cached = fillCacheMaybe();
        return Lists.newArrayList(cached.users());
    }


    @Override
    public List<User> listUsersForTenant(long tenantId) throws ApiException, TException {
        return fillCacheMaybe().listUsersForTenant(tenantId);
    }

    @Override
    public void updateUser(long userId, String identifier, String passwordHash, String secret, boolean isFdsAdmin) throws ApiException, TException {
        config.updateUser(userId, identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
    }

    @Override
    public long configurationVersion(long ignore) throws ApiException, TException {
        return fillCacheMaybe().getVersion();
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumeSettings volumeSettings, long tenantId) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumeSettings, tenantId);
        dropCache();
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        config.deleteVolume(domainName, volumeName);
        dropCache();
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        return fillCacheMaybe().volumesByName().get(volumeName);
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return Lists.newArrayList(fillCacheMaybe().volumesByName().values());
    }

    @Override
    public int registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds) throws TException {
        return config.registerStream(url, http_method, volume_names, sample_freq_seconds, duration_seconds);
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations(int ignore) throws TException {
        return config.getStreamRegistrations(ignore);
    }

    @Override
    public void deregisterStream(int registration_id) throws TException {
        config.deregisterStream(registration_id);
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


    private class Updater implements Runnable {
        @Override
        public void run() {
            while (true) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {}
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

        private CachedConfiguration obtainConfig(CachedConfiguration v) throws Exception {
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
