package com.formationds.om.webkit.rest.v08;
/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.webkit.rest.v08.events.IngestEvents;
import com.formationds.om.webkit.rest.v08.events.QueryEvents;
import com.formationds.om.webkit.rest.v08.metrics.IngestVolumeStats;
import com.formationds.om.webkit.rest.v08.metrics.QueryFirebreak;
import com.formationds.om.webkit.rest.v08.metrics.QueryMetrics;
import com.formationds.om.webkit.rest.v08.metrics.SystemHealthStatus;
import com.formationds.om.webkit.rest.v08.configuration.SystemCapabilities;
import com.formationds.om.webkit.rest.v08.platform.AddNode;
import com.formationds.om.webkit.rest.v08.platform.AddService;
import com.formationds.om.webkit.rest.v08.platform.GetNode;
import com.formationds.om.webkit.rest.v08.platform.ListNodes;
import com.formationds.om.webkit.rest.v08.platform.ListServices;
import com.formationds.om.webkit.rest.v08.platform.MutateNode;
import com.formationds.om.webkit.rest.v08.platform.MutateService;
import com.formationds.om.webkit.rest.v08.platform.RemoveNode;
import com.formationds.om.webkit.rest.v08.platform.RemoveService;
import com.formationds.om.webkit.rest.v08.snapshots.CreateSnapshot;
import com.formationds.om.webkit.rest.v08.snapshots.DeleteSnapshot;
import com.formationds.om.webkit.rest.v08.snapshots.ListSnapshots;
import com.formationds.om.webkit.rest.v08.snapshots.GetSnapshot;
import com.formationds.om.webkit.rest.v08.tenants.AssignUserToTenant;
import com.formationds.om.webkit.rest.v08.tenants.CreateTenant;
import com.formationds.om.webkit.rest.v08.tenants.ListTenants;
import com.formationds.om.webkit.rest.v08.tenants.MutateTenant;
import com.formationds.om.webkit.rest.v08.tenants.RevokeUserFromTenant;
import com.formationds.om.webkit.rest.v08.token.GrantToken;
import com.formationds.om.webkit.rest.v08.token.ReissueToken;
import com.formationds.om.webkit.rest.v08.users.CreateUser;
import com.formationds.om.webkit.rest.v08.users.CurrentUser;
import com.formationds.om.webkit.rest.v08.users.ListUsers;
import com.formationds.om.webkit.rest.v08.users.UpdatePassword;
import com.formationds.om.webkit.rest.v08.volumes.CloneVolume;
import com.formationds.om.webkit.rest.v08.volumes.CreateSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.volumes.CreateVolume;
import com.formationds.om.webkit.rest.v08.volumes.DeleteSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.volumes.DeleteVolume;
import com.formationds.om.webkit.rest.v08.volumes.GetVolume;
import com.formationds.om.webkit.rest.v08.volumes.ListSnapshotPoliciesForVolume;
import com.formationds.om.webkit.rest.v08.volumes.ListVolumes;
import com.formationds.om.webkit.rest.v08.volumes.ModifyVolume;
import com.formationds.om.webkit.rest.v08.volumes.MutateSnapshotPolicy;
import com.formationds.om.webkit.rest.v08.users.GetUser;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;
import javax.servlet.http.HttpServletResponse;

import java.util.function.Function;

/**
 * @author ptinius
 */
public class WebKitImpl {

    private static final Logger logger =
        LoggerFactory.getLogger( WebKitImpl.class );

    private static final String URL_PREFIX = "/fds/config/0.8";
    
    private WebApp webApp;

    private final String webDir;
    private final int httpPort;
    private final int httpsPort;
    private final Authenticator authenticator;
    private final Authorizer authorizer;
    private final SecretKey secretKey;

    public WebKitImpl( final Authenticator authenticator,
                       final Authorizer authorizer,
                       final String webDir,
                       final int httpPort,
                       final int httpsPort,
                       final SecretKey secretKey ) {

        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.webDir = webDir;
        this.httpPort = httpPort;
        this.httpsPort = httpsPort;
        this.secretKey = secretKey;

    }

    public void start( ) {

        final ConfigurationApi configApi = SingletonConfigAPI.instance().api();

        this.webApp = new WebApp( webDir );
        
        logger.info( "Initializing REST API 0.8..." );
        
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
        
        logger.info( "Completed REST API 0.8 initialization" );
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
    	getWebApp().route( HttpMethod.GET, URL_PREFIX + "/token", () -> new GrantToken( config, getAuthenticator(), getAuthorizer(), getSecretKey() ));
    	
    	//re-issue a token for a specific user
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/token/:user_id", (token) -> new ReissueToken( config, getSecretKey() ), getAuthorizer() );
    	
    	logger.trace( "Completed initializing authentication endpoints." );
    }
    
    /**
     * Configure all endpoints having to do with volumes
     * @param config
     */
    private void configureVolumeEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing volume endpoints..." );
    	
    	// list volumes
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes", (token) -> new ListVolumes() );
    	
    	// get a specific volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id", (token) -> new GetVolume() );
    	
    	// create a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes", (token) -> new CreateVolume( config, getAuthorizer(), token ) );
    	
    	// clone a volume.  It takes as input either a snapshot ID/obj or a time
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id", (token) -> new CloneVolume( config ) );
    	
    	// delete volume
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/volumes/:volume_id", (token) -> new DeleteVolume( config, getAuthorizer(), token ) );
    	
    	// modify a volume
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/volumes/:volume_id", (token) -> new ModifyVolume( config ) );
    	
    	// take a snapshot of a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/snapshots", (token) -> new CreateSnapshot( config ) );
    	
    	// list snapshots for a volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id/snapshots", (token) -> new ListSnapshots( config ) );
    	
    	// get a specific snapshot
    	authenticate( HttpMethod.GET, URL_PREFIX + "/snapshots/:snapshot_id", (token) -> new GetSnapshot( config ) );
    	
    	// delete a snapshot
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/snapshots/:snapshot_id", (token) -> new DeleteSnapshot( config ) );
    	
    	logger.trace( "Completed initializing volume endpoints." );
    }
    
    /**
     * Setup the URLs for dealing with snapshot policies
     * 
     * @param config
     */
    private void configureSnapshotPolicyEndpoints( ConfigurationApi config ){
    
    	logger.trace( "Initializing snapshot policy endpionts..." );
    	
    	// list the snapshot policies for a volume
    	authenticate( HttpMethod.GET, URL_PREFIX + "/volumes/:volume_id/snapshot_policies", (token) -> new ListSnapshotPoliciesForVolume( config ) );
    	
    	// add new snapshot policy to a volume
    	authenticate( HttpMethod.POST, URL_PREFIX + "/volumes/:volume_id/snapshot_policies", (token) -> new CreateSnapshotPolicy(config) );
    	
    	// delete a snapshot policy from a volume
    	authenticate( HttpMethod.DELETE, URL_PREFIX + "/volumes/:volume_id/snapshot_policies/:policy_id", (token) -> new DeleteSnapshotPolicy( config ) );
    	
    	// edit a snapshot policy for a volume
    	authenticate( HttpMethod.PUT, URL_PREFIX + "/volumes/:volume_id/snapshot_policies/:policy_id", (token) -> new MutateSnapshotPolicy( config ) );
    	
    	logger.trace( "Completed initializing snapshot policy endpoints." );
    }
    
    /**
     * Setup all the endpoints for dealing with users
     * 
     * @param config
     */
    private void configureUserEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing user endpoints..." );
    	
    	// list users
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/users", (token) -> new ListUsers( config, getSecretKey() ), getAuthorizer() );
    	
    	// create user
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/users", (token) -> new CreateUser( config, getSecretKey() ), getAuthorizer() );
    	
    	// get a specific user
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/users/:user_id", (token) -> new GetUser( config, getSecretKey() ), getAuthorizer() );
    	
    	// get current user
    	authenticate( HttpMethod.GET, URL_PREFIX + "/userinfo", (token) -> new CurrentUser( getAuthorizer(), token ) );
    	
    	//edit user
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/users/:user_id", (token) -> new UpdatePassword( config, getAuthorizer(), getSecretKey(), token ), getAuthorizer() );
    	
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
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes", (token) -> new ListNodes( config ), getAuthorizer() );
    	
    	// get one specific node
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes/:node_id", (token) -> new GetNode( config ), getAuthorizer() );
    	
    	// add a node into the system
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/nodes/:node_id", (token) -> new AddNode( config ), getAuthorizer() );
    	
    	// remove a node
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/nodes/:node_id", (token) -> new RemoveNode( config ), getAuthorizer() );
    	
    	// change a node (includes state)
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/nodes/:node_id", (token) -> new MutateNode( config ), getAuthorizer() );
    	
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
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/nodes/:node_id/services", (token) -> new ListServices( config ), getAuthorizer() );
    	
    	// add a new service to a node
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/nodes/:node_id/services", (token) -> new AddService( config ), getAuthorizer() );
    	
    	// remove a service from a node
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/nodes/:node_id/services/:service_id", (token) -> new RemoveService( config ), getAuthorizer() );
    	
    	// change a service (primarily used for start/stop)
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/nodes/:node_id/services/:service_id", (token) -> new MutateService( config ), getAuthorizer() );
    	
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
    	fdsAdminOnly( HttpMethod.GET, URL_PREFIX + "/tenants", (token) -> new ListTenants( config, getSecretKey() ), getAuthorizer() );
    	
    	// create tenant
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/tenants", (token) -> new CreateTenant( config, getSecretKey() ), getAuthorizer() );
    	
    	// delete tenant TODO: Not implemented
//    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/tenants/:tenant_id", (token) -> new DeleteTenant( config ), getAuthorizer() );
    	
    	// edit tenant TODO: Not implemented
    	fdsAdminOnly( HttpMethod.PUT, URL_PREFIX + "/tenants/:tenant_id", (token) -> new MutateTenant( config ), getAuthorizer() );
    	
    	// assign a user to a tenancy
    	fdsAdminOnly( HttpMethod.POST, URL_PREFIX + "/tenants/:tenant_id/:user_id", (token) -> new AssignUserToTenant( config, getSecretKey() ), getAuthorizer() );
    	
    	// remove a user from a tenancy
    	fdsAdminOnly( HttpMethod.DELETE, URL_PREFIX + "/tenants/:tenant_id/:user_id", (token) -> new RevokeUserFromTenant( config, getSecretKey() ), getAuthorizer() );
    	
    	logger.trace( "Completed initializing tenant endpoints." );
    }
    
    /**
     * Setup all the URL's for metrics
     * 
     * @param config
     */
    private void configureMetricsEndpoints( ConfigurationApi config ){
    	
    	logger.trace( "Initializing metrics endpoints..." );
    	
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
    
    private WebApp getWebApp(){
    	return this.webApp;
    }
    
    private Authorizer getAuthorizer(){
    	return this.authorizer;
    }
    
    private Authenticator getAuthenticator(){
    	return this.authenticator;
    }
    
    private SecretKey getSecretKey(){
    	return this.secretKey;
    }
    
    /**
     * Helper method in order to make sure only an FDS admin
     * can hit the requested URL
     * 
     * @param method
     * @param route
     * @param reqHandler
     * @param authorizer
     */
    private void fdsAdminOnly( HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> reqHandler, Authorizer authorizer ) {
        
    	authenticate( method, route, ( t ) -> {
            
            try {
                
            	if( authorizer.userFor( t )
                              .isIsFdsAdmin() ) {
                    return reqHandler.apply( t );
                } else {
                    return ( r, p ) -> {
                    	
                		JSONObject jObject = new JSONObject();
                		jObject.put( "message",  "Invalid permissions" );
                        
                		return new JsonResource( jObject, HttpServletResponse.SC_UNAUTHORIZED );
                    };
                }
            	
            } catch( SecurityException e ) {
                
            	logger.error( "Error authorizing request, userId = " + t.getUserId(), e );
                
            	return ( r, p ) -> {
            		
            		JSONObject jObject = new JSONObject();
            		jObject.put( "message",  "Invalid permissions" );
            		
                    return new JsonResource( jObject, HttpServletResponse.SC_UNAUTHORIZED );
            	};
            }
        } );
    }

    /**
     * Helper to make sure the call is authenticated
     * before passing to the requested URL
     *  
     * @param method
     * @param route
     * @param reqHandler
     */
    private void authenticate( HttpMethod method, String route, Function<AuthenticationToken,RequestHandler> reqHandler ) {
        
    	HttpAuthenticator httpAuth = new HttpAuthenticator( reqHandler, getAuthenticator() );
    	
    	HttpErrorHandler httpErrorHandler = new HttpErrorHandler( httpAuth );
        getWebApp().route( method, route, () -> httpErrorHandler );
    }
}

