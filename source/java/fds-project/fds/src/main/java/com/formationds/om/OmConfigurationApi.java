
/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.om;

import com.formationds.apis.*;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.EndUserMessages;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.protocol.svc.types.FDSP_PolicyInfoType;
import com.formationds.protocol.svc.types.FDSP_VolumeDescType;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.pm.NotifyAddServiceMsg;
import com.formationds.protocol.pm.NotifyRemoveServiceMsg;
import com.formationds.protocol.pm.NotifyStartServiceMsg;
import com.formationds.protocol.pm.NotifyStopServiceMsg;
import com.formationds.util.thrift.CachedConfiguration;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.collect.Lists;

import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;

public class OmConfigurationApi implements com.formationds.util.thrift.ConfigurationApi {
    private static final Logger LOG = Logger.getLogger( OmConfigurationApi.class );

    private final ThriftClientFactory<ConfigurationService.Iface> configClientFactory;
    private       CachedConfiguration                             cache;

    private StatStreamRegistrationHandler statStreamRegistrationHandler;

    public OmConfigurationApi( ThriftClientFactory<Iface> configClientFactory ) throws Exception {
        this.configClientFactory = configClientFactory;

        cache = new CachedConfiguration( configClientFactory.getClient() );
    }

    void createStatStreamRegistrationHandler( final String urlHostname,
                                              final int urlPortNo ) {
        this.statStreamRegistrationHandler =
            new StatStreamRegistrationHandler( this, urlHostname, urlPortNo );
    }

    void startConfigurationUpdater( long intervalMS ) {
        new Thread( new Updater( intervalMS ), "om_config_updater" ).start();
    }

    private Iface getConfig() {
        return configClientFactory.getClient();
    }

    @Override
    public long createSnapshotPolicy( final String name,
                                      final String recurrence,
                                      final long retention,
                                      final long timelineTime ) throws TException {
        final SnapshotPolicy apisPolicy = new SnapshotPolicy();

        apisPolicy.setPolicyName( name );
        apisPolicy.setRecurrenceRule( recurrence );
        apisPolicy.setRetentionTimeSeconds( retention );
        apisPolicy.setTimelineTime( timelineTime );

        return createSnapshotPolicy( apisPolicy );
    }

    /**
     * Create a Local Domain with the provided name.
     *
     * @param domainName - String: The name of the new Local Domain. Must be unique within the Global Domain.
     * @param domainSite - String: The location of the new Local Domain.
     *
     * @return long: The ID generated for the new Local Domain.
     *
     * @throws TException
     */
    @Override
    public long createLocalDomain( String domainName, String domainSite ) throws TException {
        long id = getConfig().createLocalDomain( domainName, domainSite );
        EventManager.notifyEvent( OmEvents.CREATE_LOCAL_DOMAIN, domainName, domainSite );
        return id;
    }

    /**
     * List all currently defined Local Domains (within the context of the understood Global Domain).
     *
     * @return List<com.formationds.apis.LocalDomain>: A list of the currently defined Local Domains and associated information.
     *
     * @throws TException
     */
    @Override
    public List<LocalDomainDescriptor> listLocalDomains( int ignore ) throws TException {
        return getConfig().listLocalDomains( ignore );
    }

    /**
     * Interface version V07.
     *
     * List all currently defined Local Domains (within the context of the understood Global Domain).
     *
     * @return List<com.formationds.apis.LocalDomainV07>: A list of the currently defined Local Domains and associated information.
     *
     * @throws TException
     */
    @Override
    public List<LocalDomainDescriptorV07> listLocalDomainsV07( int ignore ) throws TException {
        return getConfig().listLocalDomainsV07( ignore );
    }

    /**
     * Rename the given Local Domain.
     *
     * @param oldDomainName - String: The name of the Local Domain to be renamed.
     * @param newDomainName - String: The new name of the Local Domain.
     *
     * @throws TException
     */
    @Override
    public void updateLocalDomainName( String oldDomainName, String newDomainName ) throws TException {
        getConfig().updateLocalDomainName( oldDomainName, newDomainName );
    }

    /**
     * Rename the given Local Domain's site.
     *
     * @param domainName  - String: The name of the Local Domain whose site is to be renamed.
     * @param newSiteName - String: The new name of the Local Domain's site.
     *
     * @throws TException
     */
    @Override
    public void updateLocalDomainSite( String domainName, String newSiteName ) throws TException {
        getConfig().updateLocalDomainSite( domainName, newSiteName );
    }

    /**
     * Set the Local Domain's throttle.
     *
     * @param domainName    - String: The name of the Local Domain whose throttle is to be set.
     * @param throttleLevel - Double: The throttle level to set for the Local Domain.
     *
     * @throws TException
     */
    @Override
    public void setThrottle( String domainName, double throttleLevel ) throws TException {
        getConfig().setThrottle( domainName, throttleLevel );
    }

    /**
     * Set the Local Domain's scavenger action.
     *
     * @param domainName      - String: The name of the Local Domain whose scavenger action is to be set.
     * @param scavengerAction - String: The scavenger action to set for the Local Domain. One of
     *                        "enable", "disable", "start", "stop".
     *
     * @throws TException
     */
    @Override
    public void setScavenger( String domainName, String scavengerAction ) throws TException {
        getConfig().setScavenger( domainName, scavengerAction );
    }

    @Override
    public void startupLocalDomain( String domainName ) throws TException {
        getConfig().startupLocalDomain( domainName );
    }

    /**
     * Shutdown the given Local Domain.
     *
     * @param domainName - String: The name of the Local Domain to be shutdown.
     *
     * @throws TException
     */
    @Override
    public int shutdownLocalDomain( String domainName ) throws TException {
        return getConfig().shutdownLocalDomain( domainName );
    }

    /**
     * Delete the given Local Domain.
     *
     * @param domainName - String: The name of the Local Domain to be deleted.
     *
     * @throws TException
     */
    @Override
    public void deleteLocalDomain( String domainName ) throws TException {
        getConfig().deleteLocalDomain( domainName );
    }

    /**
     * Activate all currently defined Services on all currently defined Nodes the given Local Domain.
     * <p/>
     * If all Service flags are set to False, it will
     * be interpreted to mean activate all Services currently defined for the Node. If there are
     * no Services currently defined for the node, it will be interpreted to mean activate all
     * Services on the Node (SM, DM, and AM), and define all Services for the Node.
     *
     * @param domainName - String: The name of the Local Domain whose services are to be activated.
     * @param sm         - A boolean indicating whether the SM Service should be activated (True) or not (False)
     * @param dm         - A boolean indicating whether the DM Service should be activated (True) or not (False)
     * @param am         - A boolean indicating whether the AM Service should be activated (True) or not (False)
     *
     * @throws TException
     */
    @Override
    public void activateLocalDomainServices( String domainName,
                                             boolean sm,
                                             boolean dm,
                                             boolean am ) throws TException {
        getConfig().activateLocalDomainServices( domainName, sm, dm, am );
    }

    /**
     * Activate the specified Services on the specified Node.
     *
     * @param act_serv_req - FDSP_ActivateOneNodeType: Class identifying node and its services to be activated.
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int ActivateNode( FDSP_ActivateOneNodeType act_serv_req ) throws TException {
        return getConfig().ActivateNode( act_serv_req );
    }

    /**
     * Add service to the specified Node.
     *
     * @param add_svc_req - NotifyAddServiceMsg: Struct containing list of services
     *                    associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int AddService( NotifyAddServiceMsg add_svc_req ) throws TException {
        return getConfig().AddService( add_svc_req );
    }

    /**
     * Start service on the specified Node.
     *
     * @param start_svc_req - NotifyStartServiceMsg: Struct containing list of services
     *                      associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int StartService( NotifyStartServiceMsg start_svc_req ) throws TException {
        return getConfig().StartService( start_svc_req );
    }

    /**
     * Stop service on the specified Node.
     *
     * @param stop_svc_req - NotifyStopServiceMsg: Struct containing list of services
     *                     associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int StopService( NotifyStopServiceMsg stop_svc_req ) throws TException {
        return getConfig().StopService( stop_svc_req );
    }

    /**
     * Remove service on the specified Node.
     *
     * @param rm_svc_req - NotifyRemoveServiceMsg: Struct containing list of services
     *                   associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int RemoveService( NotifyRemoveServiceMsg rm_svc_req ) throws TException {
        return getConfig().RemoveService( rm_svc_req );
    }

    /**
     * List all currently defined Services for the given Local Domain.
     *
     * @param domainName - String: The name of the Local Domain whose services are to be listed.
     *
     * @return List<com.formationds.apis.LocalDomain>: A list of the currently defined Services for the given Local Domain.
     *
     * @throws TException
     */
    @Override
    public List<FDSP_Node_Info_Type> listLocalDomainServices( String domainName ) throws TException {
        return getConfig().listLocalDomainServices( domainName );
    }

    /**
     * List all currently defined Services for the given Local Domain.
     *
     * @return List<com.formationds.apis.LocalDomain>: A list of the currently defined Services for the only Local Domain.
     *
     * @throws TException
     */
    @Override
    public List<FDSP_Node_Info_Type> ListServices( int ignore ) throws TException {
        return getConfig().ListServices( ignore );
    }

    /**
     * Remove all currently defined Services on all currently defined Nodes the given Local Domain.
     * <p/>
     * If all Service flags are set to False, it will
     * be interpreted to mean remove all Services currently defined for the Node.
     * Removal means that the Service is unregistered from the Domain and shutdown.
     *
     * @param domainName - String: The name of the Local Domain whose services are to be removed.
     * @param sm         - A boolean indicating whether the SM Service should be removed (True) or not (False)
     * @param dm         - A boolean indicating whether the DM Service should be removed (True) or not (False)
     * @param am         - A boolean indicating whether the AM Service should be removed (True) or not (False)
     *
     * @throws TException
     */
    @Override
    public void removeLocalDomainServices( String domainName,
                                           boolean sm,
                                           boolean dm,
                                           boolean am ) throws TException {
        getConfig().removeLocalDomainServices( domainName, sm, dm, am );
    }

    /**
     * Remove the specified Services from the specified Node.
     *
     * @param rm_node_req - FDSP_RemoveServicesType: Class identifying node and its services to be removed.
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int RemoveServices( com.formationds.apis.FDSP_RemoveServicesType rm_node_req ) throws TException {
        return getConfig().RemoveServices( rm_node_req );
    }

    @Override
    public long createTenant( String identifier ) throws TException {
        long tenantId = getConfig().createTenant( identifier );
        /*
        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY);
        // TODO: XDI implementation hardcodes tenant system volume domain to "FDS_S3" (via S3Endpoint.FDS_S3. Not sure if this is correct?
        getConfig().createVolume( "FDS_S3", systemFolderName( tenantId ), volumeSettings, tenantId );
        */
        EventManager.notifyEvent( OmEvents.CREATE_TENANT, identifier );
        return tenantId;
    }

    @Override
    public List<Tenant> listTenants( int ignore ) throws TException {
        List<Tenant> t = getCache().tenants();
        if ( t == null || t.isEmpty() ) {
            t = refreshCacheMaybe().tenants();
        }
        return t;
    }

    /**
     * @param userId the user id
     *
     * @return an option containing the tenant of the specified user, if the user is assigned to a tenant
     */
    @Override
    public Optional<Tenant> tenantFor( long userId ) {
        Optional<Tenant> t = getCache().tenantFor( userId );
        if ( t == null || !t.isPresent() ) {
            t = refreshCacheMaybe().tenantFor( userId );
        }
        return t;
    }

    @Override
    public long createUser( String identifier,
                            String passwordHash,
                            String secret,
                            boolean isFdsAdmin ) throws TException {
        long userId = getConfig().createUser( identifier, passwordHash, secret, isFdsAdmin );
        EventManager.notifyEvent( OmEvents.CREATE_USER, identifier, isFdsAdmin, userId );
        return userId;
    }

    @Override
    public void assignUserToTenant( long userId, long tenantId )
        throws TException {
        getConfig().assignUserToTenant( userId, tenantId );
        EventManager.notifyEvent( OmEvents.ASSIGN_USER_TENANT, userId, tenantId );

        getCache().addTenantUser( userId, tenantId );
    }

    @Override
    public void revokeUserFromTenant( long userId, long tenantId ) throws TException {
        getConfig().revokeUserFromTenant( userId, tenantId );
        EventManager.notifyEvent( OmEvents.REVOKE_USER_TENANT, userId, tenantId );

        getCache().removeTenantUser( tenantId, userId );
    }

    @Override
    public List<User> allUsers( long ignore ) throws TException {
        Collection<User> users = getCache().users();
        if ( users == null || users.isEmpty() ) {
            users = refreshCacheMaybe().users();
        }
        return (users instanceof List ? (List<User>) users : new ArrayList<>( users ));
    }

    /**
     * @param userId the user id
     *
     * @return the tenant id the user is associated, with or null if it does not exist.
     *
     * @throws SecurityException
     */
    @Override
    public Long tenantId( long userId ) throws SecurityException {
        Long tid = getCache().tenantId( userId );
        if ( tid == null || tid == -1 ) {
            tid = refreshCacheMaybe().tenantId( userId );
        }
        return tid;
    }

    /**
     * @param userId the user id to lookup
     *
     * @return the user
     */
    @Override
    public User getUser( long userId ) {
        User u = getCache().getUser( userId );
        if ( u == null ) {
            u = refreshCacheMaybe().getUser( userId );
        }
        return u;
    }

    /**
     * @param login the login
     *
     * @return the user for the specified login
     */
    @Override
    public User getUser( String login ) {
        User u = getCache().getUser( login );
        if ( u == null ) {
            u = refreshCacheMaybe().getUser( login );
        }
        return u;
    }

    @Override
    public List<User> listUsersForTenant( long tenantId ) throws TException {
        List<User> users = getCache().listUsersForTenant( tenantId );
        if ( users == null || users.isEmpty() ) {
            users = refreshCacheMaybe().listUsersForTenant( tenantId );
        }
        return users;
    }

    @Override
    public void updateUser( long userId,
                            String identifier,
                            String passwordHash,
                            String secret,
                            boolean isFdsAdmin ) throws TException {
        getConfig().updateUser( userId, identifier, passwordHash, secret, isFdsAdmin );
        EventManager.notifyEvent( OmEvents.UPDATE_USER, identifier, isFdsAdmin, userId );

        getCache().updateUser( userId, identifier, passwordHash, secret, isFdsAdmin );
    }

    @Override
    public long configurationVersion( long ignore ) throws TException {
        return refreshCacheMaybe().getVersion();
    }

    /**
     * Create a QoS Policy with the provided name and accoutrements.
     *
     * @param policyName - String: The name of the new QoS Policy. Must be unique within the Global Domain.
     * @param iopsMin    - A long representing the minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
     *                   "Service Level Agreement".
     * @param iopsMax    - A long representing the maximum IOPS guaranteed by the policy. Also referred to as "Limit".
     * @param relPrio    - An integer representing the relative priority of requests against Volumes with this policy compared
     *                   to requests against Volumes with different relative priorities.
     *
     * @return FDSP_PolicyInfoType: The detail of the created QoS Policy.
     *
     * @throws TException
     */
    @Override
    public FDSP_PolicyInfoType createQoSPolicy( String policyName,
                                                long iopsMin,
                                                long iopsMax,
                                                int relPrio ) throws TException {
        FDSP_PolicyInfoType qosPolicy = getConfig().createQoSPolicy( policyName, iopsMin, iopsMax, relPrio );
        EventManager.notifyEvent( OmEvents.CREATE_QOS_POLICY, qosPolicy.getPolicy_name() );
        return qosPolicy;
    }

    /**
     * List all currently defined QoS Policies (within the context of the understood Global Domain).
     *
     * @return List<FDSP_PolicyInfoType>: A list of the currently defined QoS Policies and associated information.
     *
     * @throws TException
     */
    @Override
    public List<FDSP_PolicyInfoType> listQoSPolicies( long ignore ) throws TException {
        return getConfig().listQoSPolicies( ignore );
    }

    /**
     * Modify a QoS Policy with the provided name and accoutrements.
     *
     * @param currentPolicyName - String: The name of the current QoS Policy.
     * @param newPolicyName     - String: The name of the new QoS Policy. Must be unique within the Global Domain. May be
     *                          the same as currentPolicyName if the name is not changing.
     * @param iopsMin           - A long representing the new minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
     *                          "Service Level Agreement".
     * @param iopsMax           - A long representing the new maximum IOPS guaranteed by the policy. Also referred to as "Limit".
     * @param relPrio           - An integer representing the new relative priority of requests against Volumes with this policy compared
     *                          to requests against Volumes with different relative priorities.
     *
     * @return FDSP_PolicyInfoType: The detail of the modified QoS Policy.
     *
     * @throws TException
     */
    @Override
    public FDSP_PolicyInfoType modifyQoSPolicy( String currentPolicyName,
                                                String newPolicyName,
                                                long iopsMin,
                                                long iopsMax,
                                                int relPrio ) throws TException {
        FDSP_PolicyInfoType policy = getConfig().modifyQoSPolicy( currentPolicyName, newPolicyName,
                                                                  iopsMin, iopsMax, relPrio );
        EventManager.notifyEvent( OmEvents.MODIFY_QOS_POLICY, policy.getPolicy_name() );
        return policy;
    }

    /**
     * Delete the given QoS Policy.
     *
     * @param policyName - String: The name of the QoS Policy to be deleted.
     *
     * @throws TException
     */
    @Override
    public void deleteQoSPolicy( String policyName ) throws TException {
        getConfig().deleteQoSPolicy( policyName );
        EventManager.notifyEvent( OmEvents.DELETE_QOS_POLICY, policyName );
    }

    @Override
    public void createVolume( String domainName,
                              String volumeName,
                              VolumeSettings volumeSettings,
                              long tenantId ) throws TException {
        getConfig().createVolume( domainName, volumeName, volumeSettings, tenantId );

        VolumeType vt = volumeSettings.getVolumeType();
        long maxSize = (VolumeType.BLOCK.equals( vt ) ?
                        volumeSettings.getBlockDeviceSizeInBytes() :
                        volumeSettings.getMaxObjectSizeInBytes());

        statStreamRegistrationHandler.notifyVolumeCreated( domainName, volumeName );

        // load the new volume into the cache
        getCache().loadVolume( domainName, volumeName );

        EventManager.notifyEvent( OmEvents.CREATE_VOLUME, domainName, volumeName, tenantId,
                                  vt.name(),
                                  maxSize );
    }

    @Override
    public long getVolumeId( String volumeName ) throws TException {
        return getConfig().getVolumeId( volumeName );
    }

    @Override
    public String getVolumeName( long volumeId ) throws TException {
        return getConfig().getVolumeName( volumeId );
    }

    @Override
    public FDSP_VolumeDescType GetVolInfo( FDSP_GetVolInfoReqType vol_info_req ) throws TException {
        return getConfig().GetVolInfo( vol_info_req );
    }

    @Override
    public int ModifyVol( FDSP_ModifyVolType mod_vol_req ) throws TException {
        LOG.trace( "ModifyVol::" + mod_vol_req.toString() );
        return getConfig().ModifyVol( mod_vol_req );
    }

    @Override
    public void deleteVolume( String domainName, String volumeName ) throws TException {
        getConfig().deleteVolume( domainName, volumeName );
        EventManager.notifyEvent( OmEvents.DELETE_VOLUME, domainName, volumeName );
        statStreamRegistrationHandler.notifyVolumeDeleted( domainName, volumeName );

        getCache().removeVolume( domainName, volumeName );
    }

    @Override
    public VolumeDescriptor statVolume( String domainName, String volumeName ) throws TException {
        VolumeDescriptor v = getCache().getVolume( domainName, volumeName );
        if ( v == null ) {
            v = refreshCacheMaybe().getVolume( domainName, volumeName );
        }
        return v;
    }

    @Override
    public List<VolumeDescriptor> listVolumes( String domainName ) throws TException {
        // todo: may want to always do version check in refresh here.
        List<VolumeDescriptor> v = getCache().getVolumes();
        if ( v == null || v.isEmpty() ) {
            v = refreshCacheMaybe().getVolumes();
        }
        return v;
    }

    @Override
    public List<FDSP_VolumeDescType> ListVolumes( int ignore ) throws TException {
        return Lists.newArrayList( getConfig().ListVolumes( ignore ) );
    }

    @Override
    public int registerStream( String url,
                               String http_method,
                               List<String> volume_names,
                               int sample_freq_seconds,
                               int duration_seconds ) throws TException {
        return getConfig().registerStream( url, http_method, volume_names, sample_freq_seconds, duration_seconds );
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations( int ignore ) throws TException {
        return getConfig().getStreamRegistrations( ignore );
    }

    @Override
    public void deregisterStream( int registration_id ) throws TException {
        getConfig().deregisterStream( registration_id );
    }

    @Override
    public long createSnapshotPolicy( SnapshotPolicy policy ) throws TException {
        long l = getConfig().createSnapshotPolicy( policy );
        // TODO: is the value returned the new policy id?
        EventManager.notifyEvent( OmEvents.CREATE_SNAPSHOT_POLICY, policy.getPolicyName(), policy.getRecurrenceRule(),
                                  policy.getRetentionTimeSeconds(), l );
        return l;
    }

    @Override
    public List<SnapshotPolicy> listSnapshotPolicies( long unused ) throws TException {
        return getConfig().listSnapshotPolicies( unused );
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void deleteSnapshotPolicy( long id ) throws TException {
        getConfig().deleteSnapshotPolicy( id );
        EventManager.notifyEvent( OmEvents.DELETE_SNAPSHOT_POLICY, id );
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void attachSnapshotPolicy( long volumeId, long policyId ) throws TException {
        getConfig().attachSnapshotPolicy( volumeId, policyId );
        EventManager.notifyEvent( OmEvents.ATTACH_SNAPSHOT_POLICY, policyId, volumeId );
    }

    @Override
    public List<SnapshotPolicy> listSnapshotPoliciesForVolume( long volumeId ) throws TException {
        return getConfig().listSnapshotPoliciesForVolume( volumeId );
    }

    @Override
    public void detachSnapshotPolicy( long volumeId, long policyId ) throws TException {
        getConfig().detachSnapshotPolicy( volumeId, policyId );
        EventManager.notifyEvent( OmEvents.DETACH_SNAPSHOT_POLICY, policyId, volumeId );
    }

    @Override
    public List<Long> listVolumesForSnapshotPolicy( long policyId ) throws TException {
        return getConfig().listVolumesForSnapshotPolicy( policyId );
    }

    @Override
    public void createSnapshot( long volumeId,
                                String snapshotName,
                                long retentionTime,
                                long timelineTime ) throws TException {
        getConfig().createSnapshot( volumeId, snapshotName, retentionTime, timelineTime );
        // TODO: is there a generated snapshot id?
        EventManager.notifyEvent( OmEvents.CREATE_SNAPSHOT, snapshotName, volumeId, retentionTime );
    }

    @Override
    public void deleteSnapshot( long volumeId, long snapshotId ) throws TException {
        getConfig().deleteSnapshot( volumeId, snapshotId );
    }

    @Override
    public List<com.formationds.protocol.svc.types.Snapshot> listSnapshots( long volumeId ) throws TException {
        return getConfig().listSnapshots( volumeId );
    }

    @Override
    public void restoreClone( long volumeId, long snapshotId ) throws TException {
        getConfig().restoreClone( volumeId, snapshotId );
        EventManager.notifyEvent( OmEvents.RESTORE_CLONE, volumeId, snapshotId );
    }

    @Override
    public long cloneVolume( long volumeId,
                             long fdsp_PolicyInfoId,
                             String clonedVolumeName,
                             long timelineTime ) throws TException {
        long clonedVolumeId = getConfig().cloneVolume( volumeId, fdsp_PolicyInfoId, clonedVolumeName, timelineTime );
        if ( clonedVolumeId <= 0 ) {
            clonedVolumeId = getConfig().getVolumeId( clonedVolumeName );
        }

        EventManager.notifyEvent( OmEvents.CLONE_VOLUME,
                                  volumeId,
                                  fdsp_PolicyInfoId,
                                  clonedVolumeName,
                                  clonedVolumeId );

        getCache().loadVolume( clonedVolumeId );

        return clonedVolumeId;
    }

    /* Subscription Management */
    /**
     * Create a subscription.
     *
     * @param subName: Subscription name. Unique within the global domain for the given tenant.
     * @param tenantID: ID of the tenant owning the subscription.
     * @param primaryDomainID: ID of the local domain in which the primary copy of the replicated volume resides.
     * @param primaryVolumeID: ID of the volume which is the source of replication.
     * @param replicaDomainID: ID of the local domain in which the replica copy of the replicated volume resides.
     * @param subType: Indicates whether the replication mechanism for this subscription is content-based or
     *                 transaction-based.
     * @param schedType: For content-based replication, specifies what kind of scheduling mechanism is to be used.
     * @param intervalSize: For content-based replication, specifies the quantity upon which the scheduling mechanism
     *                      is based. 5 might be every 5 minutes or every 5 MB of change depending upon schedType,
     *                      for example.
     * @return ID of the created subscription. 0 if creation failed.
     * @throws TException
     */
    @Override
    public long createSubscription(String subName,
                                   long tenantID,
                                   int primaryDomainID,
                                   long primaryVolumeID,
                                   int replicaDomainID,
                                   com.formationds.apis.SubscriptionType subType,
                                   com.formationds.apis.SubscriptionScheduleType schedType,
                                   long intervalSize)
            throws TException {
        long id = getConfig().createSubscription(subName,
                                              tenantID,
                                              primaryDomainID,
                                              primaryVolumeID,
                                              replicaDomainID,
                                              subType,
                                              schedType,
                                              intervalSize);

        EventManager.notifyEvent(OmEvents.CREATE_SUBSCRIPTION,
                                 subName,
                                 tenantID,
                                 primaryDomainID,
                                 primaryVolumeID,
                                 replicaDomainID);

        return id;
    }

    /**
     * List all subscriptions defined in the global domain.
     *
     * @param ignore: Crutch for Thrift when no parameters required.
     * @return List of subscriptions and their detail. May be empty if no subscriptions defined in the global domain.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listSubscriptionsAll(int ignore)
            throws TException {
        return getConfig().listSubscriptionsAll(ignore);
    }

    /**
     * List all subscriptions defined in the global domain for the identified tenant.
     *
     * @param tenantID: ID of the tenant owning the subscriptions to be listed.
     * @return List of subscriptions owned by the tenant and their detail. May be empty if no subscriptions defined
     *         in the global domain for the identified tenant.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listTenantSubscriptionsAll(long tenantID)
            throws TException {
        return getConfig().listTenantSubscriptionsAll(tenantID);
    }

    /**
     * List all subscriptions defined for the identified primary domain.
     *
     * @param primaryDomainID: ID of the local domain that is to be identified as "primary" for the subscriptions listed.
     * @return List of subscriptions with the identified primary domain and their detail. May be empty if no
     *         subscriptions are defined for the identified primary domain.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listSubscriptionsPrimaryDomain(int primaryDomainID)
            throws TException {
        return getConfig().listSubscriptionsPrimaryDomain(primaryDomainID);
    }

    /**
     * List all subscriptions defined for the identified primary domain and tenant.
     *
     * @param primaryDomainID: ID of the local domain that is to be identified as "primary" for the subscriptions listed.
     * @param tenantID: ID of the tenant owning the subscriptions to be listed.
     * @return List of subscriptions with the identified primary domain and tenant and their detail. May be empty if no
     *         subscriptions defined for the identified primary domain and tenant.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listTenantSubscriptionsPrimaryDomain(int primaryDomainID, long tenantID)
            throws TException {
        return getConfig().listTenantSubscriptionsPrimaryDomain(primaryDomainID, tenantID);
    }

    /**
     * List all subscriptions defined for the identified replica domain.
     *
     * @param replicaDomainID: ID of the local domain that is to be identified as "replica" for the subscriptions listed.
     * @return List of subscriptions with the identified replica domain and their detail. May be empty if no
     *         subscriptions are defined for the identified replica domain.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listSubscriptionsReplicaDomain(int replicaDomainID)
            throws TException {
        return getConfig().listSubscriptionsReplicaDomain(replicaDomainID);
    }

    /**
     * List all subscriptions defined for the identified replica domain and tenant.
     *
     * @param replicaDomainID: ID of the local domain that is to be identified as "replica" for the subscriptions listed.
     * @param tenantID: ID of the tenant owning the subscriptions to be listed.
     * @return List of subscriptions with the identified replica domain and tenant and their detail. May be empty if no
     *         subscriptions defined for the identified replica domain and tenant.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listTenantSubscriptionsReplicaDomain(int replicaDomainID, long tenantID)
            throws TException {
        return getConfig().listTenantSubscriptionsReplicaDomain(replicaDomainID, tenantID);
    }

    /**
     * List all subscriptions defined with the identified volume as primary. Note that tenant is implied since
     * a volume is owned by exactly one tenant.
     *
     * @param primaryVolumeID: ID of the volume which is named as "primary" for the subscription.
     * @return List of subscriptions with the identified primary volume and their detail. May be empty if no
     *         subscriptions defined for the identified primary volume.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listSubscriptionsPrimaryVolume(long primaryVolumeID)
            throws TException {
        return getConfig().listSubscriptionsPrimaryVolume(primaryVolumeID);
    }

    /**
     * List all subscriptions defined with the identified volume as replica. Note that tenant is implied since
     * a volume is owned by exactly one tenant.
     *
     * @param replicaVolumeID: ID of the volume which is named as "replica" for the subscription.
     * @return List of subscriptions with the identified replica volume and their detail. May be empty if no
     *         subscriptions defined for the identified replica volume.
     * @throws TException
     */
    @Override
    public List<SubscriptionDescriptor> listSubscriptionsReplicaVolume(long replicaVolumeID)
            throws TException {
        return getConfig().listSubscriptionsReplicaVolume(replicaVolumeID);
    }

    /**
     * Retrieve the subscription details for the subscription identified by name and tenant (which combined is a unique
     * identifier for a subscription).
     *
     * @param subName: Subscription name. Unique within the global domain for the given tenant.
     * @param tenantID: ID of the tenant owning the subscription named.
     * @return Details of subscription for the identified tenant and with the given name.
     * @throws TException
     */
    @Override
    public SubscriptionDescriptor getSubscriptionInfoName(String subName, long tenantID)
            throws TException {
        return getConfig().getSubscriptionInfoName(subName, tenantID);
    }

    /**
     * Retrieve the subscription details for the subscription identified by the globally unique ID.
     *
     * @param subID: ID of the subscription whose details are to be retrieved.
     * @return Details of subscription identified.
     * @throws TException
     */
    @Override
    public SubscriptionDescriptor getSubscriptionInfoID(long subID)
            throws TException {
        return getConfig().getSubscriptionInfoID(subID);
    }

    /**
     * Update the indicated subscription.
     *
     * @param subMods: Using the ID included in the subMods object, replace the current subscription contents with
     *                 the contents provided here. subMods content that cannot be modified includes tenantID, primaryDomainID,
     *                 primaryVolumeID, and replicaDomainID.
     * @return Nothing.
     * @throws TException
     */
    @Override
    public void updateSubscription(com.formationds.apis.SubscriptionDescriptor subMods)
            throws TException {
        getConfig().updateSubscription(subMods);

        EventManager.notifyEvent(OmEvents.UPDATE_SUBSCRIPTION,
                                 subMods.name,
                                 subMods.tenantID,
                                 subMods.primaryDomainID,
                                 subMods.primaryVolumeID,
                                 subMods.replicaDomainID);
    }

    /**
     * Delete the subscription identified by name and tenant (which combined is a unique
     * identifier for a subscription).
     *
     * @param subName: Subscription name. Unique within the global domain for the given tenant.
     * @param tenantID: ID of the tenant owning the subscription named.
     * @param dematerialize: "true" if the replica data associated with this subscription is to be deleted as well.
     * @return Nothing.
     * @throws TException
     */
    @Override
    public void deleteSubscriptionName(String subName, long tenantID, boolean dematerialize)
            throws TException {
        getConfig().deleteSubscriptionName(subName, tenantID, dematerialize);

        String doDemat = (dematerialize) ? "with" : "without";

        EventManager.notifyEvent(OmEvents.DELETE_SUBSCRIPTION_NAME,
                                 subName,
                                 tenantID,
                                 doDemat);
    }

    /**
     * Delete the subscription identified by its globally unique ID.
     *
     * @param subID: ID of the subscription to be deleted.
     * @param dematerialize: "true" if the replica data associated with this subscription is to be deleted as well.
     * @return Nothing.
     * @throws TException
     */
    @Override
    public void deleteSubscriptionID(long subID, boolean dematerialize)
            throws TException {
        getConfig().deleteSubscriptionID(subID, dematerialize);

        String doDemat = (dematerialize) ? "with" : "without";

        EventManager.notifyEvent(OmEvents.DELETE_SUBSCRIPTION_ID,
                subID,
                doDemat);
    }

    @Override
    public List<SvcInfo> getAllNodeInfo() throws TException {
        return getConfig().getAllNodeInfo();
    }

    @Override
    public SvcInfo getNodeInfo( SvcUuid nodeUuid ) throws ApiException, TException {
        return getConfig().getNodeInfo( nodeUuid );
    }

    @Override
    public long getDiskCapacityNode( SvcUuid nodeUuid ) throws ApiException, TException {
        return getConfig().getDiskCapacityNode( nodeUuid );
    }

    @Override
    public long getDiskCapacityTotal() throws ApiException, TException {
        return getConfig().getDiskCapacityTotal();
    }

    private synchronized CachedConfiguration getCache() {
        return cache;
    }

    /**
     * Check if the configuration database version has changed and if so refresh the cache
     *
     * @return the cache
     */
    // TODO: synch here is held over a remote call to config service which is potentially problematic. consider refactoring
    private synchronized CachedConfiguration refreshCacheMaybe() {
        try {
            long currentVersion = getConfig().configurationVersion( 0 );
            long cacheVersion = cache.getVersion();
            if ( cacheVersion != currentVersion ) {
                LOG.debug( "Cache version changed - refreshing" );
                cache = new CachedConfiguration( getConfig() );
            }
            return cache;
        } catch ( TException te ) {
            throw new IllegalStateException( EndUserMessages.CS_ACCESS_DENIED, te );
        }
    }

    private class Updater implements Runnable {

        private final long intervalMS;
        private transient boolean stopRequested = false;

        Updater( long intervalMS ) {
            this.intervalMS = intervalMS;
        }

        synchronized void shutdown() {
            stopRequested = true;
            notifyAll();
        }

        @Override
        public void run() {
            // do an initial wait
            try { Thread.sleep( 10000 ); } catch ( InterruptedException ie ) {
                LOG.warn( "Received interrupt on initial wait.  Exiting cache updater thread." );
                return;
            }

            while ( !stopRequested ) {
                try {
                    synchronized ( this ) {
                        wait( intervalMS );
                        if ( stopRequested ) { break; }
                    }

                    refreshCacheMaybe();
                } catch ( InterruptedException e ) {
                    String msg = (stopRequested ? "Received stop request." : "Received interrupt.");
                    LOG.trace( msg + " Exiting cache updater thread." );
                    break;
                } catch ( Exception e ) {
                    LOG.error( "Error refreshing cache", e );
                    break;
                }
            }
        }
    }
}
