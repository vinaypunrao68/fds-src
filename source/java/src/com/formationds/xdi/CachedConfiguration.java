/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.xdi;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import org.apache.thrift.TException;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.stream.Collectors;

public class CachedConfiguration {

    private final ConfigurationService.Iface config;

    private final Lock usersLock = new ReentrantLock();
    private Map<Long, User>   usersById;
    private Map<String, User> usersByName;

    private final Map<Long, Tenant> tenantsById = new HashMap<>();
    private final Map<Long, Long> tenantsByUser = new HashMap<>();

    private final Multimap<Long, Long> usersByTenant = HashMultimap.create();

    private final Lock                          volumeLock    = new ReentrantLock();
    private final Map<String, VolumeDescriptor> volumesByName = new HashMap<>();
    private final Map<Long, VolumeDescriptor>   volumesById   = new HashMap<>();

    private final long version;

    public CachedConfiguration( ConfigurationService.Iface config ) throws Exception {
        this.config = config;

        version = config.configurationVersion( 0 );
        loadUsers();

        loadTenants();
        loadVolumes();
    }

    void loadTenants( ) {
        try {
            List<Tenant> tenants = config.listTenants( 0 );
            for ( Tenant tenant : tenants ) {
                List<User> tenantUsers = config.listUsersForTenant( tenant.getId() );
                addTenantUsers( tenant, tenantUsers );
            }
        } catch (TException t) {
            // TODO: Disregard for now, throws exception if cluster is not ready
        }
    }

    void loadVolumes() {
        List<VolumeDescriptor> volumeDescriptors;
        try {
            volumeDescriptors = config.listVolumes( "" );
        } catch ( TException e ) {
            // TODO: Disregard for now, throws exception if cluster is not ready
            volumeDescriptors = Lists.newArrayList();
        }

        volumeLock.lock();
        try {
            volumeDescriptors.forEach( v -> {
                volumesByName.put( v.getName(), v );
                volumesById.put( v.getVolId(), v );
            } );
        } finally {
            volumeLock.unlock();
        }
    }

    void loadUsers() {
        try {
            List<User> users = config.allUsers( 0 );
            usersLock.lock();
            try {
                usersByName = users.stream()
                                   .collect( Collectors.toMap( User::getIdentifier, u -> u ) );
                usersById = users.stream()
                                 .collect( Collectors.toMap( u -> u.getId(), u -> u ) );
            } finally {
                usersLock.unlock();
            }
        } catch (TException e) {
            // TODO: Disregard for now, throws exception if cluster is not ready

        }
    }

    Collection<User> users() {
        return new ArrayList<>( usersById.values() );
    }

    public User getUser(long id) {
        usersLock.lock();
        try {
            return usersById.get( id );
        } finally {
            usersLock.unlock();
        }
    }

    public User getUser(String userName) {
        usersLock.lock();
        try {
            return usersByName.get( userName );
        } finally {
            usersLock.unlock();
        }
    }

    void addUser( User user ) {
        usersLock.lock();
        try {
            usersById.put( user.getId(), user );
            usersByName.put( user.getIdentifier(), user );
        } finally {
            usersLock.unlock();
        }
    }

    void removeUser( User user ) {
        usersLock.lock();
        try {
            usersById.remove( user.getId() );
            usersByName.remove( user.getIdentifier() );
        } finally {
            usersLock.unlock();
        }
    }

    public Long tenantId(long userId) {
        return tenantsByUser.get(userId);
    }

    public List<VolumeDescriptor> getVolumes() {
        volumeLock.lock();
        try {
            return new ArrayList<>( volumesByName.values() );
        } finally {
            volumeLock.unlock();
        }
    }

    public VolumeDescriptor getVolume(String ignoredDomain, String volumeName) {
        volumeLock.lock();
        try {
            return volumesByName.get( volumeName );
        } finally {
            volumeLock.unlock();
        }
    }

    public VolumeDescriptor getVolume(long volumeId) {
        volumeLock.lock();
        try {
            return volumesById.get( volumeId );
        } finally {
            volumeLock.unlock();
        }
    }

    void addVolume(VolumeDescriptor vol) {
        volumeLock.lock();
        try {
            volumesByName.put( vol.getName(), vol );
            volumesById.put( vol.getVolId(), vol );
        } finally {
            volumeLock.unlock();
        }
    }

    void removeVolume(VolumeDescriptor vol) {
        removeVolume( "", vol.getName() );
    }

    void removeVolume( String domainName, String volumeName) {
        volumeLock.lock();
        try {
            VolumeDescriptor vol = volumesByName.remove( volumeName );
            if (vol != null) {
                volumesById.remove( vol.getVolId() );
            }
        } finally {
            volumeLock.unlock();
        }
    }

    void addTenant(Tenant tenant) {
        synchronized ( tenantsById ) {
            tenantsById.put(tenant.getId(), tenant);
        }
    }

    void addTenantUsers(Tenant tenant,  User... users) {
        if (users != null && users.length > 0) {
            addTenantUsers( tenant, Arrays.asList( users ) );
        } else {
            // just add the tenant
            addTenant( tenant );
        }
    }

    void addTenantUsers(Tenant tenant,  List<User> tenantUsers) {
        synchronized ( tenantsById ) {
            tenantsById.put( tenant.getId(), tenant );
            usersByTenant.putAll( tenant.getId(),
                                  tenantUsers.stream()
                                             .map( u -> u.getId() ).collect( Collectors.toSet() ) );
            for ( User tenantUser : tenantUsers ) {
                tenantsByUser.put( tenantUser.getId(), tenant.getId() );
            }
        }
    }

    void addTenantUser(long tid, long uid) {
        synchronized ( tenantsById ) {
            Tenant tenant = tenantsById.get( tid );
            usersByTenant.put( tid, uid );
            tenantsByUser.put( uid, tid );
        }
    }

    void removeTenantUser(long tid, long uid) {
        synchronized ( tenantsById ) {
            usersByTenant.remove( tid, uid );
            tenantsByUser.remove( uid, tid );
        }
    }


    void removeTenant(long tid) {
        synchronized ( tenantsById ) {
            usersByTenant.removeAll( tid );
            Iterator<Long> titer = tenantsByUser.values().iterator();
            while (titer.hasNext()) {
                Long t = titer.next();
                if (t == tid) {
                    titer.remove();
                }
            }
            tenantsById.remove( tid );
        }
    }

    public List<Tenant> tenants() {
        return new ArrayList<>( tenantsById.values() );
    }

    public List<User> listUsersForTenant(long tenantId) {
        usersLock.lock();
        try {
            return usersByTenant.get( tenantId ).stream()
                                .map( id -> usersById.get( id ) )
                                .collect( Collectors.toList() );
        } finally {
            usersLock.unlock();
        }
    }

    public Optional<Tenant> tenantFor(long userId) {
        synchronized (tenantsById) {
            if ( tenantsByUser.containsKey( userId ) ) {
                long tenantId = tenantsByUser.get( userId );
                return Optional.of( tenantsById.get( tenantId ) );
            } else {
                return Optional.empty();
            }
        }
    }

    public long getVersion() {
        return version;
    }
}
