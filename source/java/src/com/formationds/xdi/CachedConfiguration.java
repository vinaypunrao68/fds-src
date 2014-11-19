package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import org.apache.thrift.TException;

import java.util.*;
import java.util.stream.Collectors;

public class CachedConfiguration {

    private final List<User> users;
    private final List<Tenant> tenants;
    private final Map<Long, Tenant> tenantsById;
    private final Multimap<Long, Long> usersByTenant;
    private final Map<Long, Long> tenantsByUser;
    private final Map<String, VolumeDescriptor> volumesByName;
    private final Map<Long, User> usersById;
    private final Map<String, User> usersByName;
    private final long version;
    private List<VolumeDescriptor> volumeDescriptors;

    public CachedConfiguration(ConfigurationService.Iface config) throws Exception {
        version = config.configurationVersion(0);
        users = config.allUsers(0);
        usersByName = users.stream()
                .collect(Collectors.toMap(u -> u.getIdentifier(), u -> u));

        usersById = users.stream().collect(Collectors.toMap(u -> u.getId(), u -> u));
        tenants = config.listTenants(0);
        usersByTenant = HashMultimap.create();
        tenantsById = new HashMap<>();

        tenantsByUser = new HashMap<>();
        for (Tenant tenant : tenants) {
            tenantsById.put(tenant.getId(), tenant);
            List<User> tenantUsers = config.listUsersForTenant(tenant.getId());
            usersByTenant.putAll(tenant.getId(), tenantUsers.stream().map(u -> u.getId()).collect(Collectors.toSet()));
            for (User tenantUser : tenantUsers) {
                tenantsByUser.put(tenantUser.getId(), tenant.getId());
            }
        }

        // Disregard for now, throws exception if cluster is not ready
        try {
            volumeDescriptors = config.listVolumes("");
        } catch (TException e) {
            volumeDescriptors = Lists.newArrayList();
        }
        volumesByName = new HashMap<>();
        volumeDescriptors.forEach(v -> volumesByName.put(v.getName(), v));
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

    public Optional<Tenant> tenantFor(long userId) {
        if (tenantsByUser.containsKey(userId)) {
            long tenantId = tenantsByUser.get(userId);
            return Optional.of(tenantsById.get(tenantId));
        } else {
            return Optional.empty();
        }
    }

    public long getVersion() {
        return version;
    }
}
