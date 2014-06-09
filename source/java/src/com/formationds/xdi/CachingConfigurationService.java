package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import org.apache.thrift.TException;

import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

public class CachingConfigurationService implements ConfigurationService.Iface {
    private final ConfigurationService.Iface service;
    private final ConcurrentHashMap<Key, VolumeDescriptor> map;

    public CachingConfigurationService(ConfigurationService.Iface service) {
        this.service = service;
        map = new ConcurrentHashMap<>();
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumeSettings volumeSettings) throws ApiException, TException {
        service.createVolume(domainName, volumeName, volumeSettings);
        map.clear();
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        service.deleteVolume(domainName, volumeName);
        map.clear();
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        Key key = new Key(domainName, volumeName);
        return map.compute(key, (k, v) -> {
            if (v == null) {
                try {
                    v = service.statVolume(domainName, volumeName);
                } catch (TException e) {
                    throw new RuntimeException(e);
                }
            }

            return v;
        });
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return service.listVolumes(domainName);
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
