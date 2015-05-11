
/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.om;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.SnapshotPolicy;
import com.formationds.apis.StreamingRegistrationMsg;
import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventDescriptor;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.om.events.EventManager;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.FDSP_PolicyInfoType;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
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
        map.compute(KEY, (k, v) -> {
            try {
                return new ConfigurationCache(configClientFactory.getClient());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        });
    }

    void startStatStreamRegistrationHandler( final String urlHostname, final int urlPortNo) {
        this.statStreamRegistrationHandler = new StatStreamRegistrationHandler( this, urlHostname, urlPortNo);
        this.statStreamRegistrationHandler.start();
    }

    void startConfigurationUpdater() {
        new Thread(new Updater(),"om_config_updater").start();
    }

    private Iface getConfig() {
        return configClientFactory.getClient();
    }

    public static String systemFolderName(long tenantId) {
        return "SYSTEM_VOLUME_" + tenantId;
    }

    enum ConfigEvent implements EventDescriptor {

        CREATE_LOCAL_DOMAIN(EventCategory.SYSTEM, "Created local domain {0}", "domainName", "domainSite"),
        CREATE_TENANT(EventCategory.VOLUMES, "Created tenant {0}", "identifier"),
        CREATE_USER(EventCategory.SYSTEM, "Created user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
        UPDATE_USER(EventCategory.SYSTEM, "Updated user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
        ASSIGN_USER_TENANT(EventCategory.SYSTEM, "Assigned user {0} to tenant {1}", "userId", "tenantId"),
        REVOKE_USER_TENANT(EventCategory.SYSTEM, "Revoked user {0} from tenant {1}", "userId", "tenantId"),
        CREATE_QOS_POLICY(EventCategory.VOLUMES, "Created QoS policy {0}", "policyName"),
        MODIFY_QOS_POLICY(EventCategory.VOLUMES, "Modified QoS policy {0}", "policyName"),
        DELETE_QOS_POLICY(EventCategory.VOLUMES, "Deleted QoS policy {0}", "policyName"),
        CREATE_VOLUME(EventCategory.VOLUMES, "Created volume: domain={0}; name={1}; tenantId={2}; type={3}; size={4}",
                      "domainName", "volumeName", "tenantId", "volumeType", "maxSize"),
        DELETE_VOLUME(EventCategory.VOLUMES, "Deleted volume: domain={0}; name={1}", "domainName", "volumeName"),
        CREATE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Created snapshot policy {0} {1} {2}; id={3}",
                               "name", "recurrence", "retention", "id"),
        DELETE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Deleted snapshot policy: id={0}", "id"),
        ATTACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Attached snapshot policy {0} to volume id {1}",
                               "policyId", "volumeId"),
        DETACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Detached snapshot policy {0} from volume id {1}",
                               "policyId", "volumeId"),
        CREATE_SNAPSHOT(EventCategory.VOLUMES, "Created snapshot {0} of volume {1}.  Retention time = {2}",
                        "snapshotName", "volumeId", "retentionTime"),

        CLONE_VOLUME(EventCategory.VOLUMES,
                     "Cloned volume {0} with policy id {1}; clone name={2}; Cloned volume Id = {3}",
                     "volumeId", "policyId", "clonedVolumeName", "clonedVolumeId"),
        RESTORE_CLONE(EventCategory.VOLUMES, "Restored cloned volume {0} with snapshot id {1}", "volumeId",
                      "snapshotId");

        private final EventType     type;
        private final EventCategory category;
        private final EventSeverity severity;
        private final String        defaultMessage;
        private final List<String>  argNames;

        private ConfigEvent(EventCategory category, String defaultMessage, String... argNames) {
            this(EventType.USER_ACTIVITY, category, EventSeverity.CONFIG, defaultMessage, argNames);
        }

        private ConfigEvent(EventType type, EventCategory category, EventSeverity severity,
                            String defaultMessage, String... argNames) {
            this.type = type;
            this.category = category;
            this.severity = severity;
            this.defaultMessage = defaultMessage;
            this.argNames = (argNames != null ? Arrays.asList(argNames) : new ArrayList<String>(0));
        }

        public String key() { return name(); }

        public List<String> argNames() { return argNames; }

        public EventType type() { return type; }

        public EventCategory category() { return category; }

        public EventSeverity severity() { return severity; }

        public String defaultMessage() { return defaultMessage; }

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
        EventManager.notifyEvent(ConfigEvent.CREATE_LOCAL_DOMAIN, domainName, domainSite);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_TENANT, identifier);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_USER, identifier, isFdsAdmin, userId);
        return userId;
    }

    @Override
    public void assignUserToTenant(long userId, long tenantId)
        throws TException {
        getConfig().assignUserToTenant(userId, tenantId);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.ASSIGN_USER_TENANT, userId, tenantId);
    }

    @Override
    public void revokeUserFromTenant(long userId, long tenantId)
        throws TException {
        getConfig().revokeUserFromTenant( userId, tenantId );
        dropCache();
        EventManager.notifyEvent(ConfigEvent.REVOKE_USER_TENANT, userId, tenantId);
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
        EventManager.notifyEvent(ConfigEvent.UPDATE_USER, identifier, isFdsAdmin, userId);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_QOS_POLICY, qosPolicy.policy_name);
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
        EventManager.notifyEvent(ConfigEvent.MODIFY_QOS_POLICY, policy.policy_name);
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
        EventManager.notifyEvent(ConfigEvent.DELETE_QOS_POLICY, policyName);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_VOLUME, domainName, volumeName, tenantId,
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
    public void deleteVolume(String domainName, String volumeName)
        throws TException {
        getConfig().deleteVolume(domainName, volumeName);
        dropCache();
        EventManager.notifyEvent(ConfigEvent.DELETE_VOLUME, domainName, volumeName);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_SNAPSHOT_POLICY, policy.getPolicyName(), policy.getRecurrenceRule(),
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
        EventManager.notifyEvent(ConfigEvent.DELETE_SNAPSHOT_POLICY, id);
    }

    // TODO need deleteSnapshotForVolume Iface call.

    @Override
    public void attachSnapshotPolicy(long volumeId, long policyId)
        throws ApiException, org.apache.thrift.TException {
        getConfig().attachSnapshotPolicy(volumeId, policyId);
        EventManager.notifyEvent(ConfigEvent.ATTACH_SNAPSHOT_POLICY, policyId, volumeId);
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
        EventManager.notifyEvent(ConfigEvent.DETACH_SNAPSHOT_POLICY, policyId, volumeId);
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
        EventManager.notifyEvent(ConfigEvent.CREATE_SNAPSHOT, snapshotName, volumeId, retentionTime);
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
        EventManager.notifyEvent(ConfigEvent.RESTORE_CLONE, volumeId, snapshotId);
    }

    @Override
    public long cloneVolume(long volumeId, long fdsp_PolicyInfoId, String clonedVolumeName, long timelineTime)
        throws org.apache.thrift.TException {
        long clonedVolumeId = getConfig().cloneVolume(volumeId, fdsp_PolicyInfoId, clonedVolumeName, timelineTime);
        if (clonedVolumeId <= 0) {
            clonedVolumeId = getConfig().getVolumeId(clonedVolumeName);
        }
        dropCache();

        EventManager.notifyEvent(ConfigEvent.CLONE_VOLUME, volumeId, fdsp_PolicyInfoId, clonedVolumeName, clonedVolumeId);
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
