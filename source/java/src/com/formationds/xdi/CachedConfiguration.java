package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class CachedConfiguration {

    private final List<User> users;
    private final List<Tenant> tenants;
    private final Multimap<Long, Long> usersByTenant;
    private final Map<Long, Long> tenantsByUser;
    private final List<VolumeDescriptor> volumeDescriptors;
    private final Map<String, VolumeDescriptor> volumesByName;
    private final Map<Long, User> usersById;
    private final Map<String, User> usersByName;
    private final long version;

    public CachedConfiguration(ConfigurationService.Iface config) throws Exception {
        version = config.configurationVersion(0);
        users = config.allUsers(0);
        usersByName = users.stream()
                .collect(Collectors.toMap(u -> u.getIdentifier(), u -> u));

        usersById = users.stream().collect(Collectors.toMap(u -> u.getId(), u -> u));
        tenants = config.listTenants(0);
        usersByTenant = HashMultimap.create();
        tenantsByUser = new HashMap<>();
        for (Tenant tenant : tenants) {
            List<User> tenantUsers = config.listUsersForTenant(tenant.getId());
            usersByTenant.putAll(tenant.getId(), tenantUsers.stream().map(u -> u.getId()).collect(Collectors.toSet()));
            for (User tenantUser : tenantUsers) {
                tenantsByUser.put(tenantUser.getId(), tenant.getId());
            }
        }
        volumeDescriptors = config.listVolumes("");
        volumesByName = volumeDescriptors.stream()
                .collect(Collectors.toMap(v -> v.getName(), v -> v));
    }

    public Collection<User> users() {
        return users;
    }

    public long tenantId(long userId) throws SecurityException {
        return tenantsByUser.get(userId);
    }

    public boolean hasAccess(long userId, String volumeName) {
        if (!volumesByName.containsKey(volumeName)) {
            return false;
        }

        if (!usersById.containsKey(userId)) {
            return false;
        }

        VolumeDescriptor volumeDescriptor = volumesByName.get(volumeName);
        long tenantId = volumeDescriptor.getTenantId();
        return usersByTenant.get(tenantId).contains(userId);
    }

    public Map<String, User> usersByName() {
        return usersByName;
    }

    public Map<Long, User> usersById() {
        return usersById;
    }

    public Map<String, VolumeDescriptor> volumesByName() {
        return volumesByName;
    }

    public List<Tenant> tenants() {
        return tenants;
    }

    public List<User> listUsersForTenant(long tenantId) {
        return usersByTenant.get(tenantId).stream()
                .map(id -> usersById.get(id))
                .collect(Collectors.toList());
    }

    public long getVersion() {
        return version;
    }
}
