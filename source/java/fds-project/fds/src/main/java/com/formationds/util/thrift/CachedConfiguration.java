/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.util.thrift;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import org.apache.thrift.TException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
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

    public CachedConfiguration( ConfigurationService.Iface config ) throws TException {
        this.config = config;

        version = config.configurationVersion( 0 );
        load();
    }

    public void load()
    {
        loadUsers();
        loadTenants();
        loadVolumes();
    }

    public void loadTenants() {
        try {
            List<Tenant> tenants = config.listTenants( 0 );
            for ( Tenant tenant : tenants ) {
                List<User> tenantUsers = config.listUsersForTenant( tenant.getId() );
                addTenantUsers( tenant, tenantUsers );
            }
        } catch (TException t) {
            throw new IllegalStateException( "configuration database is not ready!.", t );
        }
    }

    public void loadVolumes() {
        List<VolumeDescriptor> volumeDescriptors;
        try {
            volumeDescriptors = config.listVolumes( "" );
        } catch ( TException e ) {
            throw new IllegalStateException( "configuration database is not ready!.", e );
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

    public void loadVolume( String unusedDomain, String volumeName ) {

        try {
            VolumeDescriptor volumeDescriptor = config.statVolume( unusedDomain, volumeName );
            volumeLock.lock();
            try {
                volumesByName.put( volumeDescriptor.getName(), volumeDescriptor );
                volumesById.put( volumeDescriptor.getVolId(), volumeDescriptor );
            } finally {
                volumeLock.unlock();
            }
        } catch ( TException e ) {
            throw new IllegalStateException( "configuration database is not ready!", e );
        }
    }

    public void loadVolume( long volumeId ) {
        try {
            String vname = config.getVolumeName( volumeId );
            loadVolume( "unused", vname );
        } catch ( TException e ) {
            throw new IllegalStateException( "configuration database is not ready!", e );
        }
    }
    public void loadUsers() {
        try {
            List<User> users = config.allUsers( 0 );
            usersLock.lock();
            try {
                usersByName = users.stream()
                                   .collect( Collectors.toMap( User::getIdentifier, u -> u ) );
                usersById = users.stream()
                                 .collect( Collectors.toMap( User::getId, u -> u ) );
            } finally {
                usersLock.unlock();
            }
        } catch (TException e) {
              throw new IllegalStateException( "configuration database is not ready!.", e );

        }
    }

    public Collection<User> users() {
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

    public void addUser( User user ) {
        usersLock.lock();
        try {
            usersById.put( user.getId(), user );
            usersByName.put( user.getIdentifier(), user );
        } finally {
            usersLock.unlock();
        }
    }

    @SuppressWarnings( "unused" )
    public void removeUser( User user ) {
        usersLock.lock();
        try {
            usersById.remove( user.getId() );
            usersByName.remove( user.getIdentifier() );

            tenantsByUser.remove( user.getId() );
        } finally {
            usersLock.unlock();
        }
    }

    public void updateUser( long userId,
                            String identifier,
                            String passwordHash,
                            String secret,
                            boolean isFdsAdmin ) {
        User user = getUser( userId );
        if ( user == null ) {
            loadUsers();
            user = getUser( userId );
            if ( user == null ) {
                // user really doesn't exist
                return;
            }
        }
        user.setIdentifier( identifier );
        user.setPasswordHash( passwordHash );
        user.setSecret( secret );
        user.setIsFdsAdmin( isFdsAdmin );
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

    public VolumeDescriptor getVolume(@SuppressWarnings( "unused" ) String ignoredDomain,
                                      String volumeName) {
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

    public void addVolume( VolumeDescriptor vol ) {
        volumeLock.lock();
        try {
            volumesByName.put( vol.getName(), vol );
            volumesById.put( vol.getVolId(), vol );
        } finally {
            volumeLock.unlock();
        }
    }

    @SuppressWarnings( "unused" )
    public void removeVolume( VolumeDescriptor vol ) {
        removeVolume( "", vol.getName() );
    }

    public void removeVolume( @SuppressWarnings("unused") String domainName,
                       String volumeName) {
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

    public void addTenant( Tenant tenant ) {
        synchronized ( tenantsById ) {
            tenantsById.put(tenant.getId(), tenant);
        }
    }

    @SuppressWarnings( "unused" )
    public void addTenantUsers( Tenant tenant, User... users ) {
        if (users != null && users.length > 0) {
            addTenantUsers( tenant, Arrays.asList( users ) );
        } else {
            // just add the tenant
            addTenant( tenant );
        }
    }

    public void addTenantUsers( Tenant tenant, List<User> tenantUsers ) {
        synchronized ( tenantsById ) {
            tenantsById.put( tenant.getId(), tenant );
            usersByTenant.putAll( tenant.getId(),
                                  tenantUsers.stream()
                                             .map( User::getId ).collect( Collectors.toSet() ) );
            for ( User tenantUser : tenantUsers ) {
                tenantsByUser.put( tenantUser.getId(), tenant.getId() );
            }
        }
    }

    public void addTenantUser( long tid, long uid ) {
        synchronized ( tenantsById ) {
            @SuppressWarnings( "unused" ) Tenant tenant = tenantsById.get( tid );
            usersByTenant.put( tid, uid );
            tenantsByUser.put( uid, tid );
        }
    }

    public void removeTenantUser( long tid, long uid ) {
        synchronized ( tenantsById ) {
            usersByTenant.remove( tid, uid );
            tenantsByUser.remove( uid, tid );
        }
    }

    @SuppressWarnings( "unused" )
    public void removeTenant( long tid ) {
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
                                .map( usersById::get )
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
