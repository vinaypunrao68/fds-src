package com.formationds.xdi;
    /*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.streaming.StreamingRegistrationMsg;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

public class CachingConfigurationService implements ConfigurationService.Iface {
    private final ConfigurationService.Iface service;
    private final ConcurrentHashMap<Key, VolumeDescriptor> volumeCache;

    public CachingConfigurationService(ConfigurationService.Iface service) {
        this.service = service;
        volumeCache = new ConcurrentHashMap<>();
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumeSettings volumeSettings, long tenantId) throws ApiException, TException {
        service.createVolume(domainName, volumeName, volumeSettings, 0);
        volumeCache.clear();
    }

    @Override
    public long createTenant(String identifier) throws ApiException, TException {
        return service.createTenant(identifier);
    }

    @Override
    public List<Tenant> listTenants(int ignore) throws ApiException, TException {
        return service.listTenants(ignore);
    }

    @Override
    public long createUser(String identifier, ByteBuffer secret, boolean isFdsAdmin) throws ApiException, TException {
        return service.createUser(identifier, secret, isFdsAdmin);
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId) throws ApiException, TException {
        service.assignUserToTenant(userId, tenantId);
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId) throws ApiException, TException {
        service.revokeUserFromTenant(userId, tenantId);
    }

    @Override
    public List<User> allUsers(long unused) throws ApiException, TException {
        return service.allUsers(unused);
    }

    @Override
    public List<User> listUsersForTenant(long tenantId) throws ApiException, TException {
        return service.listUsersForTenant(tenantId);
    }

    @Override
    public void updateUser(long userId, String identifier, ByteBuffer secret, boolean isFdsAdmin) throws ApiException, TException {
        service.updateUser(userId, identifier, secret, isFdsAdmin);
    }

    @Override
    public long configurationVersion(long ignore) throws ApiException, TException {
        return service.configurationVersion(ignore);
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        service.deleteVolume(domainName, volumeName);
        volumeCache.clear();
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        return getFromCache(domainName, volumeName);
    }

    private VolumeDescriptor getFromCache(String domainName, String volumeName) {
        Key key = new Key(domainName, volumeName);
        return volumeCache.computeIfAbsent(key, k -> {
            try {
                return service.statVolume(domainName, volumeName);
            } catch (TException e) {
                return null;
            }
        });
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return service.listVolumes(domainName);
    }

    @Override
    public int registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds) throws org.apache.thrift.TException {
        return service.registerStream(url, http_method, volume_names, sample_freq_seconds, duration_seconds);
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations(int thriftSucks) throws org.apache.thrift.TException {
        return service.getStreamRegistrations(thriftSucks);
    }

    @Override
    public void deregisterStream(int registration_id) throws org.apache.thrift.TException {
        service.deregisterStream(registration_id);
    }


    private class Key {
        private String domain;
        private String volumeName;

        private Key(String domain, String volumeName) {
            this.domain = domain;
            this.volumeName = volumeName;
        }

        public String getDomain() {
            return domain;
        }

        public String getVolumeName() {
            return volumeName;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Key key = (Key) o;

            if (!domain.equals(key.domain)) return false;
            if (!volumeName.equals(key.volumeName)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = domain.hashCode();
            result = 31 * result + volumeName.hashCode();
            return result;
        }
    }
}
