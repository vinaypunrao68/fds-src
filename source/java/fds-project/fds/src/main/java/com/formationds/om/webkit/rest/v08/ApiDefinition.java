/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.webkit.AbstractApiDefinition;
import com.formationds.om.webkit.rest.v08.configuration.SystemCapabilities;
import com.formationds.om.webkit.rest.v08.domain.ListLocalDomains;
import com.formationds.om.webkit.rest.v08.domain.MutateLocalDomain;
import com.formationds.om.webkit.rest.v08.events.IngestEvents;
import com.formationds.om.webkit.rest.v08.events.QueryEvents;
import com.formationds.om.webkit.rest.v08.metrics.IngestVolumeStats;
import com.formationds.om.webkit.rest.v08.metrics.MessageBusForwarder;
import com.formationds.om.webkit.rest.v08.metrics.QueryFirebreak;
import com.formationds.om.webkit.rest.v08.metrics.QueryMetrics;
import com.formationds.om.webkit.rest.v08.metrics.SystemHealthStatus;
import com.formationds.om.webkit.rest.v08.platform.AddNode;
import com.formationds.om.webkit.rest.v08.platform.AddService;
import com.formationds.om.webkit.rest.v08.platform.GetNode;
import com.formationds.om.webkit.rest.v08.platform.GetService;
import com.formationds.om.webkit.rest.v08.platform.ListNodes;
import com.formationds.om.webkit.rest.v08.platform.ListServices;
import com.formationds.om.webkit.rest.v08.platform.MutateNode;
import com.formationds.om.webkit.rest.v08.platform.MutateService;
import com.formationds.om.webkit.rest.v08.platform.RemoveNode;
import com.formationds.om.webkit.rest.v08.platform.RemoveService;
import com.formationds.om.webkit.rest.v08.presets.GetDataProtectionPolicyPresets;
import com.formationds.om.webkit.rest.v08.presets.GetQosPolicyPresets;
import com.formationds.om.webkit.rest.v08.snapshots.CreateSnapshot;
import com.formationds.om.webkit.rest.v08.snapshots.GetSnapshot;
import com.formationds.om.webkit.rest.v08.snapshots.ListSnapshots;
import com.formationds.om.webkit.rest.v08.stream.DeregisterStream;
import com.formationds.om.webkit.rest.v08.stream.ListStreams;
import com.formationds.om.webkit.rest.v08.stream.RegisterStream;
import com.formationds.om.webkit.rest.v08.tenants.AssignUserToTenant;
import com.formationds.om.webkit.rest.v08.tenants.CreateTenant;
import com.formationds.om.webkit.rest.v08.tenants.ListTenants;
import com.formationds.om.webkit.rest.v08.tenants.RevokeUserFromTenant;
import com.formationds.om.webkit.rest.v08.token.GrantToken;
import com.formationds.om.webkit.rest.v08.token.ReissueToken;
import com.formationds.om.webkit.rest.v08.token.StatsUserAuth;
import com.formationds.om.webkit.rest.v08.token.StatsVhostAuth;
import com.formationds.om.webkit.rest.v08.token.StatsResourceAuth;
import com.formationds.om.webkit.rest.v08.users.CreateUser;
import com.formationds.om.webkit.rest.v08.users.CurrentUser;
import com.formationds.om.webkit.rest.v08.users.GetUser;
import com.formationds.om.webkit.rest.v08.users.ListUsers;
import com.formationds.om.webkit.rest.v08.users.ListUsersForTenant;
import com.formationds.om.webkit.rest.v08.users.UpdatePassword;
import com.formationds.om.webkit.rest.v08.volumes.CloneVolumeFromSnapshot;
import com.formationds.om.webkit.rest.v08.volumes.CloneVolumeFromTime;
import com.formationds.om.webkit.rest.v08.volumes.CreateSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.volumes.CreateVolume;
import com.formationds.om.webkit.rest.v08.volumes.DeleteSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.volumes.DeleteVolume;
import com.formationds.om.webkit.rest.v08.volumes.GetVolume;
import com.formationds.om.webkit.rest.v08.volumes.GetVolumeTypes;
import com.formationds.om.webkit.rest.v08.volumes.ListSnapshotPoliciesForVolume;
import com.formationds.om.webkit.rest.v08.volumes.ListVolumes;
import com.formationds.om.webkit.rest.v08.volumes.MutateSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.volumes.MutateVolume;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;

/**
 * @author ptinius
 */
public class ApiDefinition extends AbstractApiDefinition{

    private static final Logger logger =
        LoggerFactory.getLogger( ApiDefinition.class );

    private static final String URL_PREFIX = "/fds/config/v08";
    
    private WebApp webApp;

    private final Authenticator authenticator;
    private final Authorizer authorizer;
    private final SecretKey secretKey;

    public ApiDefinition( final Authenticator authenticator,
                       final Authorizer authorizer,
                       final SecretKey secretKey,
                       WebApp webApp) {

        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.secretKey = secretKey;
        this.webApp = webApp;

    }

    public void configure() {

        final ConfigurationApi configApi = SingletonConfigAPI.instance().api();
        
        configureCapabilities( configApi );
        configureAuthEndpoints( configApi );
        configureVolumeEndpoints( configApi );
        configureSnapshotPolicyEndpoints( configApi );
        configureUserEndpoints( configApi );
        configureNodeEndpoints( configApi );
        configureServiceEndpoints( configApi );
        configureTenantEndpoints( configApi );
        configureMetricsEndpoints( configApi );
        configureEventsEndpoints( configApi );
        configureDomainEndpoints();
        configurePresetEndpoints();
        configureMessageBusAuthEndpoint();
        
    }
    
    private void configureMessageBusAuthEndpoint(){
	
    	getWebApp().route( HttpMethod.GET, URL_PREFIX + "/stats/auth/user", () -> new StatsUserAuth( this.authenticator, this.authorizer, this.secretKey ) );
    	getWebApp().route( HttpMethod.GET, URL_PREFIX + "/stats/auth/vhost", () -> new StatsVhostAuth() );
    	getWebApp().route( HttpMethod.GET, URL_PREFIX + "/stats/auth/resources", () -> new StatsResourceAuth() );
    	
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/mb/:route", ( t ) -> new MessageBusForwarder() );
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/mb/:route", ( t ) -> new MessageBusForwarder() );
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/mb/:route", ( t ) -> new MessageBusForwarder() );
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/mb/:route", ( t ) -> new MessageBusForwarder() );
    }
    
    /**
     * Configure all endpoints needed to deal with capabilities of the system
     * 
     * @param config
     */
    private void configureCapabilities( ConfigurationApi config ){
    	
    	logger.trace( "Initializing capabilities URLs..." );
    	
    	ParsedConfig platformConfig = SingletonConfiguration.instance().getConfig().getPlatformConfig();
	
		authenticate( HttpMethod.GET, URL_PREFIX + "/capabilities", ( t ) -> new SystemCapabilities( platformConfig ) );
		
		logger.trace( "Completed initializing capabilities URLs." );
    }
    
    /**
     * Configure all endpoints having to do with tokens and authentication
     */
    private void configureAuthEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing authentication endpoints..." );
    	
    	// login / get an token using credentials
    	getWebApp().route( HttpMethod.POST, URL_PREFIX + "/token", () -> new GrantToken( config, getAuthenticator(), getAuthorizer(), getSecretKey() ));
    	
    	//re-issue a token for a specific user
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/token/:user_id", (token) -> new ReissueToken( getSecretKey() ) );
    	
    	logger.trace( "Completed initializing authentication endpoints." );
    }
    
    /**
     * Configure all endpoints having to do with volumes
     * @param config
     */
    private void configureVolumeEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing volume endpoints..." );
    	
    	// list volumes
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes", (token) -> new ListVolumes( getAuthorizer(), token ) );
    	
    	// list the available types
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volume_types", (token) -> new GetVolumeTypes() );
    	
    	// get a specific volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id", (token) -> new GetVolume( this.authorizer, token ) );
    	
    	// create a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes", (token) -> new CreateVolume( getAuthorizer(), token ) );
    	
    	// clone a volume from snapshot ID
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/snapshot/:snapshot_id", (token) -> new CloneVolumeFromSnapshot( this.authorizer, token ) );

    	// clone a volume from a time
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/time/:time_in_seconds", (token) -> new CloneVolumeFromTime( this.authorizer, token) );
    	
    	// delete volume
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/volumes/:volume_id", (token) -> new DeleteVolume( getAuthorizer(), token ) );
    	
    	// modify a volume
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/volumes/:volume_id", (token) -> new MutateVolume( getAuthorizer(), token ) );
    	
    	// take a snapshot of a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/snapshots", (token) -> new CreateSnapshot() );
    	
    	// list snapshots for a volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id/snapshots", (token) -> new ListSnapshots() );
    	
    	// get a specific snapshot
    	authenticate( HttpMethod.GET, URL_PREFIX + "/snapshots/:snapshot_id", (token) -> new GetSnapshot( ) );
    	
    	// delete a snapshot
      // TODO implement delete snapshot
//    	authenticate( HttpMethod.DELETE,
//                    URL_PREFIX + "/snapshots/:snapshot_id",
//                    ( token ) -> new DeleteSnapshot( config ) );
    	
    	logger.trace( "Completed initializing volume endpoints." );
    }
    
    /**
     * Setup the URLs for dealing with snapshot policies
     * 
     * @param config
     */
    private void configureSnapshotPolicyEndpoints( ConfigurationApi config ){
    
    	logger.trace( "Initializing snapshot policy endpoints..." );
    	
    	// list the snapshot policies for a volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id/snapshot_policies", (token) -> new ListSnapshotPoliciesForVolume() );
    	
    	// add new snapshot policy to a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/snapshot_policies", (token) -> new CreateSnapshotPolicy( this.authorizer, token ) );
    	
    	// delete a snapshot policy from a volume
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/volumes/:volume_id/snapshot_policies/:policy_id", (token) -> new DeleteSnapshotPolicy() );

    	// edit a snapshot policy for a volume
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/volumes/:volume_id/snapshot_policies/:policy_id", (token) -> new MutateSnapshotPolicy() );
    	
    	logger.trace( "Completed initializing snapshot policy endpoints." );
    }
    
    /**
     * Setup all the endpoints for dealing with presets
     */
    private void configurePresetEndpoints(){
    	
    	// the list of quality of service preset policies
    	authenticate( HttpMethod.GET, URL_PREFIX + "/presets/quality_of_service_policies", (token) -> new GetQosPolicyPresets() );
    	
    	// the list of data protection policies
    	authenticate( HttpMethod.GET, URL_PREFIX + "/presets/data_protection_policies", (token) -> new GetDataProtectionPolicyPresets() );
    }
    
    /**
     * Setup all the endpoints for dealing with users
     * 
     * @param config
     */
    private void configureUserEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing user endpoints..." );
    	
    	// list users
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/users", (token) -> new ListUsers() );
    	
    	// list users for tenant
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/users/tenant/:tenant_id", (token) -> new ListUsersForTenant() );
    	
    	// create user
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/users", (token) -> new CreateUser() );
    	
    	// get a specific user
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/users/:user_id", (token) -> new GetUser() );
    	
    	// get current user
    	authenticate( HttpMethod.GET, URL_PREFIX + "/userinfo", (token) -> new CurrentUser( getAuthorizer(), token ) );
    	
    	//edit user
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/users/:user_id", (token) -> new UpdatePassword( getAuthorizer(), token ) );
    	
    	// delete user  TODO: Not implemented yet
//    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/users/:user_id", (token) -> new DeleteUser( config ), getAuthorizer() );
    	
    	logger.trace( "Completed initializing user endpoints." );
    }
    
    /**
     * Setup all the endpoints for interacting with nodes
     * 
     * @param config
     */
    private void configureNodeEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing node endpoints..." );
    	
    	// list nodes and services
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes", (token) -> new ListNodes() );
    	
    	// get one specific node
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes/:node_id", (token) -> new GetNode() );
    	
    	// add a node into the system
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/nodes/:node_id", (token) -> new AddNode() );
    	
    	// remove a node
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/nodes/:node_id", (token) -> new RemoveNode() );
    	
    	// change a node (includes state)
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/nodes/:node_id", (token) -> new MutateNode() );
    	
    	logger.trace( "Completed initializing node endpoints." );
    }
    
    /**
     * Setup the URLs for service management
     * 
     * @param config
     */
    private void configureServiceEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing service endpoints..." );
    	
    	// list services on a node
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes/:node_id/services", (token) -> new ListServices() );
    	
    	// get a specific service
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes/:node_id/services/:service_id", (token) -> new GetService() );
    	
    	// add a new service to a node
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/nodes/:node_id/services", (token) -> new AddService() );
    	
    	// remove a service from a node
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/nodes/:node_id/services/:service_id", (token) -> new RemoveService() );
    	
    	// change a service (primarily used for start/stop)
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/nodes/:node_id/services/:service_id", (token) -> new MutateService() );
    	
    	logger.trace( "Completed initializing service endpoints." );
    }
    
    /**
     * Setup all the URLs for tenant management
     * 
     * @param config
     */
    private void configureTenantEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing tenant endpoints..." );
    	
    	// list the tenants
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/tenants", (token) -> new ListTenants( ) );
    	
    	// create tenant
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/tenants", (token) -> new CreateTenant() );
    	
    	// delete tenant TODO: Not implemented
//    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/tenants/:tenant_id", (token) -> new DeleteTenant( config ) );
    	
    	// edit tenant TODO: Not implemented
//    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/tenants/:tenant_id", (token) -> new MutateTenant( ) );
    	
    	// assign a user to a tenancy
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/tenants/:tenant_id/:user_id", (token) -> new AssignUserToTenant() );
    	
    	// remove a user from a tenancy
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/tenants/:tenant_id/:user_id", (token) -> new RevokeUserFromTenant() );
    	
    	logger.trace( "Completed initializing tenant endpoints." );
    }
    
    /**
     * Setup all the URL's for domains
     */
    private void configureDomainEndpoints(){
    	
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/local_domains", (token) -> new ListLocalDomains() );
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/local_domains/:domain_id", (token) -> new MutateLocalDomain() ); 
    }
    
    /**
     * Setup all the URL's for metrics
     * 
     * @param config
     */
    private void configureMetricsEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing metrics endpoints..." );
    	
    	// register a stats stream
    	authenticate( HttpMethod.POST, URL_PREFIX + "/stats/streams", (token) -> new RegisterStream() );
    	
    	// De-register a stats stream
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/stats/streams", (token) -> new DeregisterStream() );
    	
    	// get a list of registered stream
    	authenticate( HttpMethod.GET, URL_PREFIX + "/stats/streams", (token) -> new ListStreams() );
    	
    	// get volume activity statistics
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/stats/volumes", (token) -> new QueryMetrics( getAuthorizer(), token ) );
  
    	// get special firebreak statistics for a volume
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/stats/volumes/firebreak", (token) -> new QueryFirebreak( getAuthorizer(), token ) );
  
    	// get a system health check object
    	authenticate( HttpMethod.GET,  URL_PREFIX + "/systemhealth", (token) -> new SystemHealthStatus( config, getAuthorizer(), token ) );
    	
    	// post new stats into the system
        getWebApp().route( HttpMethod.POST, URL_PREFIX + "/stats", () -> new IngestVolumeStats( config ) );
        
        logger.trace( "Completed initializing metrics endpoints." );
    }
    
    /**
     * Setup all the endpoints for events
     * 
     * @param config
     */
    private void configureEventsEndpoints( ConfigurationApi config ){

    	logger.trace( "Initializing events endpoints..." );
    	
    	// TODO: only the AM should be sending this event to us. How can we validate that?
        getWebApp().route( HttpMethod.POST, URL_PREFIX + "/events", () -> new IngestEvents() );

        // get a list of events that match a filters
        authenticate( HttpMethod.PUT, URL_PREFIX + "/events", (token) -> new QueryEvents() );
        
        logger.trace( "Completed initializing events endpoints." );
    }
    
// Accessors for debug-ability    
    
    protected WebApp getWebApp(){
    	return this.webApp;
    }
    
    protected Authorizer getAuthorizer(){
    	return this.authorizer;
    }
    
    protected Authenticator getAuthenticator(){
    	return this.authenticator;
    }
    
    protected SecretKey getSecretKey(){
    	return this.secretKey;
    }
}

