
/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.om;

import com.formationds.apis.*;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.FDSP_PolicyInfoType;
import com.formationds.protocol.FDSP_VolumeDescType;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;
import java.util.stream.Collectors;

public class OmConfigurationApi implements com.formationds.util.thrift.ConfigurationApi {
    private static final Logger LOG = Logger.getLogger(OmConfigurationApi.class);
    private static final long   KEY = 0;

    class ConfigurationCache {
        private final List<User>                    users;
        private final List<Tenant>                  tenants;
        private final Map<Long, Tenant>             tenantsById;
        private final Multimap<Long, Long>          usersByTenant;
        private final Map<Long, Long>               tenantsByUser;
        private       List<VolumeDescriptor>        volumeDescriptors;
        private final Map<String, VolumeDescriptor> volumesByName;
        private final Map<Long, User>               usersById;
        private final Map<String, User>             usersByName;
        private final long                          version;

        public ConfigurationCache(ConfigurationService.Iface config) throws Exception {
            version = config.configurationVersion(0);
            users = config.allUsers(0);
            usersByName = users.stream().collect(Collectors.toMap( ( Function<User, String> ) User::getIdentifier, u -> u));
            usersById = users.stream().collect(Collectors.toMap( User::getId, u -> u));
            tenants = config.listTenants(0);
            usersByTenant = HashMultimap.create();
            tenantsById = new HashMap<>();

            tenantsByUser = new HashMap<>();
            for (Tenant tenant : tenants) {
                tenantsById.put(tenant.getId(), tenant);
                List<User> tenantUsers = config.listUsersForTenant(tenant.getId());
                usersByTenant
                    .putAll(tenant.getId(), tenantUsers.stream().map( User::getId ).collect(Collectors.toSet()));
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
            volumesByName = volumeDescriptors.stream().collect(Collectors.toMap( VolumeDescriptor::getName, v -> v));
        }

        public Collection<User> users() { return users; }
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

        public Map<String, User> usersByName() { return usersByName; }
        public Map<Long, User> usersById() { return usersById; }
        public Map<String, VolumeDescriptor> volumesByName() { return volumesByName; }

        public List<Tenant> tenants() { return tenants; }

        /**
         *
         * @param userId
         * @return the tenantid the specified user is associated with.  Null if not found.
         *
         * @throws SecurityException
         */
        public Long tenantId(long userId) throws SecurityException { return tenantsByUser.get(userId); }

        public List<User> listUsersForTenant(long tenantId) {
            return usersByTenant.get(tenantId).stream()
                                .map( usersById::get )
                                .collect(Collectors.toList());
        }

        /**
         *
         * @param userId
         * @return the optional tenant for the specified user id
         */
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

    private final ThriftClientFactory<ConfigurationService.Iface>  configClientFactory;
    private final ConcurrentHashMap<Long, ConfigurationCache> map;

    private StatStreamRegistrationHandler statStreamRegistrationHandler;

    public OmConfigurationApi(ThriftClientFactory<Iface> configClientFactory) throws Exception {
        this.configClientFactory = configClientFactory;

        map = new ConcurrentHashMap<>();
        map.compute( KEY, ( k, v ) -> {
            try
            {
                return new ConfigurationCache(
                    configClientFactory.getClient( ) );
            }
            catch( Exception e )
            {
                throw new RuntimeException( e );
            }
        } );
    }

    void createStatStreamRegistrationHandler( final String urlHostname,
                                              final int urlPortNo ) {
        this.statStreamRegistrationHandler =
            new StatStreamRegistrationHandler( this, urlHostname, urlPortNo);
    }

    void startConfigurationUpdater() {
        new Thread(new Updater(),"om_config_updater").start();
    }

    private Iface getConfig() {
        return configClientFactory.getClient();
    }

    @Override
    public long createSnapshotPolicy(final String name, final String recurrence, final long retention,
                                     final long timelineTime)
        throws TException {
        final SnapshotPolicy apisPolicy = new SnapshotPolicy();

        apisPolicy.setPolicyName(name);
        apisPolicy.setRecurrenceRule(recurrence);
        apisPolicy.setRetentionTimeSeconds(retention);
        apisPolicy.setTimelineTime(timelineTime);

        return createSnapshotPolicy(apisPolicy);
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
    public long createLocalDomain(String domainName, String domainSite)
        throws TException {
        long id = getConfig().createLocalDomain(domainName, domainSite);
        EventManager.notifyEvent(OmEvents.CREATE_LOCAL_DOMAIN, domainName, domainSite);
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
    public List<com.formationds.apis.LocalDomain> listLocalDomains(int ignore)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listLocalDomains(ignore);
    }
    
    /**
     * Rename the given Local Domain.
     * 
     * @param oldDomainName - String: The name of the Local Domain to be renamed.
     * @param newDomainName - String: The new name of the Local Domain.
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void updateLocalDomainName(String oldDomainName, String newDomainName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().updateLocalDomainName(oldDomainName, newDomainName);
        return;
    }
    
    /**
     * Rename the given Local Domain's site.
     * 
     * @param domainName - String: The name of the Local Domain whose site is to be renamed.
     * @param newSiteName - String: The new name of the Local Domain's site.
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void updateLocalDomainSite(String domainName, String newSiteName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().updateLocalDomainSite(domainName, newSiteName);
        return;
    }
    
    /**
     * Set the Local Domain's throttle.
     * 
     * @param domainName - String: The name of the Local Domain whose throttle is to be set.
     * @param throttleLevel - Double: The throttle level to set for the Local Domain.
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void setThrottle(String domainName, double throttleLevel)
        throws ApiException, org.apache.thrift.TException {
        getConfig().setThrottle(domainName, throttleLevel);
        return;
    }
    
    /**
     * Set the Local Domain's scavenger action.
     * 
     * @param domainName - String: The name of the Local Domain whose scavenger action is to be set.
     * @param scavengerAction - String: The scavenger action to set for the Local Domain. One of
     *                          "enable", "disable", "start", "stop".
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void setScavenger(String domainName, String scavengerAction)
        throws ApiException, org.apache.thrift.TException {
        getConfig().setScavenger(domainName, scavengerAction);
        return;
    }
    
    @Override
    public void startupLocalDomain(String domainName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().startupLocalDomain(domainName);
        return;
    }
    
    /**
     * Shutdown the given Local Domain.
     * 
     * @param domainName - String: The name of the Local Domain to be shutdown.
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void shutdownLocalDomain(String domainName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().shutdownLocalDomain(domainName);
        return;
    }
    
    /**
     * Delete the given Local Domain.
     * 
     * @param domainName - String: The name of the Local Domain to be deleted.
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void deleteLocalDomain(String domainName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().deleteLocalDomain(domainName);
        return;
    }
    
    /**
     * Activate all currently defined Services on all currently defined Nodes the given Local Domain.
     * 
     * If all Service flags are set to False, it will
     * be interpreted to mean activate all Services currently defined for the Node. If there are
     * no Services currently defined for the node, it will be interpreted to mean activate all
     * Services on the Node (SM, DM, and AM), and define all Services for the Node.
     *
     * @param domainName - String: The name of the Local Domain whose services are to be activated.
     * @param sm - A boolean indicating whether the SM Service should be activated (True) or not (False)
     * @param dm - A boolean indicating whether the DM Service should be activated (True) or not (False)
     * @param am - A boolean indicating whether the AM Service should be activated (True) or not (False)
     * 
     * @return void.
     * 
     * @throws TException
     */
    @Override
    public void activateLocalDomainServices(String domainName, boolean sm, boolean dm, boolean am)
        throws ApiException, org.apache.thrift.TException {
        getConfig().activateLocalDomainServices(domainName, sm, dm, am);
        return;
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
    public int ActivateNode(com.formationds.apis.FDSP_ActivateOneNodeType act_serv_req)
            throws org.apache.thrift.TException {
        return getConfig().ActivateNode(act_serv_req);
    }
    
    /**
     * Add service to the specified Node.
     *
     * @param add_svc_req - NotifyAddServiceMsg: Struct containing list of services 
     * associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int AddService(com.formationds.protocol.pm.NotifyAddServiceMsg add_svc_req)
            throws org.apache.thrift.TException {
    	return getConfig().AddService(add_svc_req);
    }
    
    /**
     * Start service on the specified Node.
     *
     * @param start_svc_req - NotifyStartServiceMsg: Struct containing list of services
     * associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int StartService(com.formationds.protocol.pm.NotifyStartServiceMsg start_svc_req)
            throws org.apache.thrift.TException {
    	return getConfig().StartService(start_svc_req);
    }
    
    /**
     * Stop service on the specified Node.
     *
     * @param stop_svc_req - NotifyStopServiceMsg: Struct containing list of services
     * associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int StopService(com.formationds.protocol.pm.NotifyStopServiceMsg stop_svc_req)
            throws org.apache.thrift.TException {
    	return getConfig().StopService(stop_svc_req);
    }
    
    /**
     * Remove service on the specified Node.
     *
     * @param rm_svc_req - NotifyRemoveServiceMsg: Struct containing list of services
     * associated with node
     *
     * @return int 0 is successful. Not 0 otherwise.
     *
     * @throws TException
     */
    @Override
    public int RemoveService(com.formationds.protocol.pm.NotifyRemoveServiceMsg rm_svc_req)
            throws org.apache.thrift.TException {
    	return getConfig().RemoveService(rm_svc_req);
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
    public List<FDSP_Node_Info_Type> listLocalDomainServices(String domainName)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listLocalDomainServices(domainName);
    }

    /**
     * List all currently defined Services for the given Local Domain.
     *
     * @return List<com.formationds.apis.LocalDomain>: A list of the currently defined Services for the only Local Domain.
     *
     * @throws TException
     */
    @Override
    public List<FDSP_Node_Info_Type> ListServices(int ignore)
            throws org.apache.thrift.TException {
        return getConfig().ListServices(ignore);
    }

    /**
     * Remove all currently defined Services on all currently defined Nodes the given Local Domain.
     *
     * If all Service flags are set to False, it will
     * be interpreted to mean remove all Services currently defined for the Node.
     * Removal means that the Service is unregistered from the Domain and shutdown.
     *
     * @param domainName - String: The name of the Local Domain whose services are to be removed.
     * @param sm - A boolean indicating whether the SM Service should be removed (True) or not (False)
     * @param dm - A boolean indicating whether the DM Service should be removed (True) or not (False)
     * @param am - A boolean indicating whether the AM Service should be removed (True) or not (False)
     *
     * @return void.
     *
     * @throws TException
     */
    @Override
    public void removeLocalDomainServices(String domainName, boolean sm, boolean dm, boolean am)
        throws ApiException, org.apache.thrift.TException {
        getConfig().removeLocalDomainServices(domainName, sm, dm, am);
        return;
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
    public int RemoveServices(com.formationds.apis.FDSP_RemoveServicesType rm_node_req)
            throws org.apache.thrift.TException {
        return getConfig().RemoveServices(rm_node_req);
    }

    @Override
    public long createTenant(String identifier)
        throws TException {
        long tenantId = getConfig().createTenant(identifier);
        /*
        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY);
        // TODO: XDI implementation hardcodes tenant system volume domain to "FDS_S3" (via S3Endpoint.FDS_S3. Not sure if this is correct?
        getConfig().createVolume( "FDS_S3", systemFolderName( tenantId ), volumeSettings, tenantId );
        */
        dropCache();
        EventManager.notifyEvent(OmEvents.CREATE_TENANT, identifier);
        return tenantId;
    }

    @Override
    public List<Tenant> listTenants(int ignore) {
        return fillCacheMaybe().tenants();
    }

    /**
     * @param userId
     * @return
     * @throws TException
     */
    @Override
    public Optional<Tenant> tenantFor(long userId) {
        return fillCacheMaybe().tenantFor( userId );
    }

    @Override
    public long createUser(String identifier, String passwordHash, String secret, boolean isFdsAdmin)
        throws TException {
        long userId = getConfig().createUser(identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
        EventManager.notifyEvent(OmEvents.CREATE_USER, identifier, isFdsAdmin, userId);
        return userId;
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId)
        throws TException {
        getConfig().assignUserToTenant(userId, tenantId);
        dropCache();
        EventManager.notifyEvent(OmEvents.ASSIGN_USER_TENANT, userId, tenantId);
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId)
        throws TException {
        getConfig().revokeUserFromTenant( userId, tenantId );
        dropCache();
        EventManager.notifyEvent(OmEvents.REVOKE_USER_TENANT, userId, tenantId);
    }

    @Override
    public List<User> allUsers(long ignore)
        throws TException {
        return Lists.newArrayList(get().users());
    }

    /**
     * @param userId
     * @return the tenant id the user is associated, with or null if it does not exist.
     * @throws SecurityException
     */
    @Override
    public Long tenantId(long userId) throws SecurityException {
        return get().tenantsByUser.get( userId );
    }

    /**
     *
     * @param userId
     * @return
     */
    @Override
    public User getUser(long userId) {
        return get().usersById().get( userId );
    }

    /**
     *
     * @param login
     * @return
     */
    @Override
    public User getUser(String login) {
        return get().usersByName().get(login);
    }

    @Override
    public List<User> listUsersForTenant(long tenantId) {
        return fillCacheMaybe().listUsersForTenant( tenantId );
    }

    @Override
    public void updateUser(long userId, String identifier,
                           String passwordHash, String secret,
                           boolean isFdsAdmin) throws TException {
        getConfig().updateUser(userId, identifier, passwordHash, secret, isFdsAdmin);
        dropCache();
        EventManager.notifyEvent(OmEvents.UPDATE_USER, identifier, isFdsAdmin, userId);
    }

    @Override
    public long configurationVersion(long ignore) throws ApiException {
        return fillCacheMaybe().getVersion();
    }

    /**
     * Create a QoS Policy with the provided name and accoutrements.
     *
     * @param policyName - String: The name of the new QoS Policy. Must be unique within the Global Domain.
     * @param iopsMin - A long representing the minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
     *                  "Service Level Agreement".
     * @param iopsMax - A long representing the maximum IOPS guaranteed by the policy. Also referred to as "Limit".
     * @param relPrio - An integer representing the relative priority of requests against Volumes with this policy compared
     *                  to requests against Volumes with different relative priorities.
     *
     * @return FDSP_PolicyInfoType: The detail of the created QoS Policy.
     *
     * @throws TException
     */
    @Override
    public FDSP_PolicyInfoType createQoSPolicy(String policyName, long iopsMin, long iopsMax, int relPrio)
            throws TException {
        FDSP_PolicyInfoType qosPolicy = getConfig().createQoSPolicy(policyName, iopsMin, iopsMax, relPrio);
        EventManager.notifyEvent(OmEvents.CREATE_QOS_POLICY, qosPolicy.policy_name);
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
    public List<FDSP_PolicyInfoType> listQoSPolicies(long ignore)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listQoSPolicies(ignore);
    }

    /**
     * Modify a QoS Policy with the provided name and accoutrements.
     *
     * @param currentPolicyName - String: The name of the current QoS Policy.
     * @param newPolicyName - String: The name of the new QoS Policy. Must be unique within the Global Domain. May be
     *                                the same as currentPolicyName if the name is not changing.
     * @param iopsMin - A long representing the new minimum IOPS to be achieved by the policy. Also referred to as "SLA" or
     *                  "Service Level Agreement".
     * @param iopsMax - A long representing the new maximum IOPS guaranteed by the policy. Also referred to as "Limit".
     * @param relPrio - An integer representing the new relative priority of requests against Volumes with this policy compared
     *                  to requests against Volumes with different relative priorities.
     *
     * @return FDSP_PolicyInfoType: The detail of the modified QoS Policy.
     *
     * @throws TException
     */
    @Override
    public FDSP_PolicyInfoType modifyQoSPolicy(String currentPolicyName, String newPolicyName,
                                               long iopsMin, long iopsMax, int relPrio)
            throws TException {
        FDSP_PolicyInfoType policy = getConfig().modifyQoSPolicy(currentPolicyName, newPolicyName,
                                                                 iopsMin, iopsMax, relPrio);
        EventManager.notifyEvent(OmEvents.MODIFY_QOS_POLICY, policy.policy_name);
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
    public void deleteQoSPolicy(String policyName)
        throws ApiException, org.apache.thrift.TException {
        getConfig().deleteQoSPolicy(policyName);
        EventManager.notifyEvent(OmEvents.DELETE_QOS_POLICY, policyName);
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumeSettings volumeSettings, long tenantId)
        throws TException {
        getConfig().createVolume(domainName, volumeName, volumeSettings, tenantId);
        dropCache();

        VolumeType vt = volumeSettings.getVolumeType();
        long maxSize = (VolumeType.BLOCK.equals(vt) ?
                        volumeSettings.getBlockDeviceSizeInBytes() :
                        volumeSettings.getMaxObjectSizeInBytes());
        EventManager.notifyEvent(OmEvents.CREATE_VOLUME, domainName, volumeName, tenantId,
                                 vt.name(),
                                 maxSize);
        statStreamRegistrationHandler.notifyVolumeCreated( domainName, volumeName );
    }

    @Override
    public long getVolumeId(String volumeName)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().getVolumeId(volumeName);
    }

    @Override
    public String getVolumeName(long volumeId)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().getVolumeName( volumeId );
    }

    @Override
    public FDSP_VolumeDescType GetVolInfo(FDSP_GetVolInfoReqType vol_info_req)
            throws org.apache.thrift.TException {
        return getConfig().GetVolInfo(vol_info_req);
    }

    @Override
    public int ModifyVol(FDSP_ModifyVolType mod_vol_req)
            throws TException {
        return getConfig().ModifyVol(mod_vol_req);
    }

    @Override
    public void deleteVolume(String domainName, String volumeName)
        throws TException {
        getConfig().deleteVolume(domainName, volumeName);
        dropCache();
        EventManager.notifyEvent(OmEvents.DELETE_VOLUME, domainName, volumeName);
        statStreamRegistrationHandler.notifyVolumeDeleted( domainName, volumeName );
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName)
        throws TException {
        return fillCacheMaybe().volumesByName()
                               .get(volumeName);
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName)
        throws TException {
        return Lists.newArrayList( fillCacheMaybe().volumesByName()
                                                   .values() );
    }

    @Override
    public List<FDSP_VolumeDescType> ListVolumes(int ignore)
            throws TException {
        return Lists.newArrayList( getConfig().ListVolumes(ignore) );
    }

    @Override
    public int registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds)
        throws TException {
        return getConfig().registerStream( url, http_method, volume_names, sample_freq_seconds, duration_seconds );
    }

    @Override
    public List<StreamingRegistrationMsg> getStreamRegistrations(int ignore)
        throws TException {
        return getConfig().getStreamRegistrations( ignore );
    }

    @Override
    public void deregisterStream(int registration_id)
        throws TException {
        getConfig().deregisterStream(registration_id);
    }

    @Override
    public long createSnapshotPolicy(com.formationds.apis.SnapshotPolicy policy)
        throws ApiException, org.apache.thrift.TException {
        long l = getConfig().createSnapshotPolicy(policy);
        // TODO: is the value returned the new policy id?
        EventManager.notifyEvent(OmEvents.CREATE_SNAPSHOT_POLICY, policy.getPolicyName(), policy.getRecurrenceRule(),
                                 policy.getRetentionTimeSeconds(), l);
        return l;
    }

    @Override
    public List<com.formationds.apis.SnapshotPolicy> listSnapshotPolicies(long unused)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listSnapshotPolicies(unused);
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void deleteSnapshotPolicy(long id)
        throws ApiException, org.apache.thrift.TException {
        getConfig().deleteSnapshotPolicy(id);
        EventManager.notifyEvent(OmEvents.DELETE_SNAPSHOT_POLICY, id);
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void attachSnapshotPolicy(long volumeId, long policyId)
        throws ApiException, org.apache.thrift.TException {
        getConfig().attachSnapshotPolicy(volumeId, policyId);
        EventManager.notifyEvent(OmEvents.ATTACH_SNAPSHOT_POLICY, policyId, volumeId);
    }

    @Override
    public List<com.formationds.apis.SnapshotPolicy> listSnapshotPoliciesForVolume(long volumeId)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listSnapshotPoliciesForVolume(volumeId);
    }

    @Override
    public void detachSnapshotPolicy(long volumeId, long policyId)
        throws ApiException, org.apache.thrift.TException {
        getConfig().detachSnapshotPolicy(volumeId, policyId);
        EventManager.notifyEvent(OmEvents.DETACH_SNAPSHOT_POLICY, policyId, volumeId);
    }

    @Override
    public List<Long> listVolumesForSnapshotPolicy(long policyId)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listVolumesForSnapshotPolicy(policyId);
    }

    @Override
    public void createSnapshot(long volumeId, String snapshotName, long retentionTime, long timelineTime)
        throws TException {
        getConfig().createSnapshot(volumeId, snapshotName, retentionTime, timelineTime);
        // TODO: is there a generated snapshot id?
        EventManager.notifyEvent(OmEvents.CREATE_SNAPSHOT, snapshotName, volumeId, retentionTime);
    }

    @Override
    public void deleteSnapshot(long volumeId, long snapshotId)
        throws TException {
        getConfig().deleteSnapshot(volumeId, snapshotId);
    }

    @Override
    public List<com.formationds.protocol.Snapshot> listSnapshots(long volumeId)
        throws ApiException, org.apache.thrift.TException {
        return getConfig().listSnapshots(volumeId);
    }

    @Override
    public void restoreClone(long volumeId, long snapshotId)
        throws ApiException, org.apache.thrift.TException {
        getConfig().restoreClone(volumeId, snapshotId);
        EventManager.notifyEvent(OmEvents.RESTORE_CLONE, volumeId, snapshotId);
    }

    @Override
    public long cloneVolume(long volumeId, long fdsp_PolicyInfoId, String clonedVolumeName, long timelineTime)
        throws org.apache.thrift.TException {
        long clonedVolumeId = getConfig().cloneVolume(volumeId, fdsp_PolicyInfoId, clonedVolumeName, timelineTime);
        if (clonedVolumeId <= 0) {
            clonedVolumeId = getConfig().getVolumeId(clonedVolumeName);
        }
        dropCache();

        EventManager.notifyEvent(OmEvents.CLONE_VOLUME, volumeId, fdsp_PolicyInfoId, clonedVolumeName, clonedVolumeId);
        return clonedVolumeId;
    }

    private ConfigurationCache get() {
        return fillCacheMaybe();
    }

    private void dropCache() {
        map.clear();
    }

    private ConfigurationCache fillCacheMaybe() {
        return map.computeIfAbsent(KEY, k -> {
            try {
                return new ConfigurationCache(getConfig());
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
                    // FIXME: In the very rare circumstance where this is okay, it must be
                    //        commented. Send review to davec@.
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

        private ConfigurationCache obtainConfig(ConfigurationCache v)
            throws Exception {
            if (v == null) {
                return new ConfigurationCache(getConfig());
            } else {
                long currentVersion = getConfig().configurationVersion(0);
                if (currentVersion != v.getVersion()) {
                    LOG.debug("Cache updated, refreshing");
                    return new ConfigurationCache(getConfig());
                } else {
                    return v;
                }
            }
        }
    }

}
