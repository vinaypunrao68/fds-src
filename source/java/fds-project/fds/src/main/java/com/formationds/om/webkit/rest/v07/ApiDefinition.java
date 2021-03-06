/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.webkit.AbstractApiDefinition;
import com.formationds.om.webkit.rest.v07.config.SystemCapabilities;
import com.formationds.om.webkit.rest.v07.domain.DeleteLocalDomain;
import com.formationds.om.webkit.rest.v07.domain.GetLocalDomainServices;
import com.formationds.om.webkit.rest.v07.domain.GetLocalDomains;
import com.formationds.om.webkit.rest.v07.domain.PostLocalDomain;
import com.formationds.om.webkit.rest.v07.domain.PutLocalDomain;
import com.formationds.om.webkit.rest.v07.domain.PutLocalDomainServices;
import com.formationds.om.webkit.rest.v07.domain.PutScavenger;
import com.formationds.om.webkit.rest.v07.domain.PutThrottle;
import com.formationds.om.webkit.rest.v07.domain.ShowGlobalDomain;
import com.formationds.om.webkit.rest.v07.events.IngestEvents;
import com.formationds.om.webkit.rest.v07.events.QueryEvents;
import com.formationds.om.webkit.rest.v07.metrics.IngestVolumeStats;
import com.formationds.om.webkit.rest.v07.metrics.QueryFirebreak;
import com.formationds.om.webkit.rest.v07.metrics.QueryMetrics;
import com.formationds.om.webkit.rest.v07.metrics.SystemHealthStatus;
import com.formationds.om.webkit.rest.v07.platform.AddNode;
import com.formationds.om.webkit.rest.v07.platform.AddService;
import com.formationds.om.webkit.rest.v07.platform.ListNodes;
import com.formationds.om.webkit.rest.v07.platform.MutateNode;
import com.formationds.om.webkit.rest.v07.platform.MutateService;
import com.formationds.om.webkit.rest.v07.platform.RemoveNode;
import com.formationds.om.webkit.rest.v07.platform.RemoveService;
import com.formationds.om.webkit.rest.v07.policy.DeleteQoSPolicy;
import com.formationds.om.webkit.rest.v07.policy.GetQoSPolicies;
import com.formationds.om.webkit.rest.v07.policy.PostQoSPolicy;
import com.formationds.om.webkit.rest.v07.policy.PutQoSPolicy;
import com.formationds.om.webkit.rest.v07.snapshot.AttachSnapshotPolicyIdToVolumeId;
import com.formationds.om.webkit.rest.v07.snapshot.CloneSnapshot;
import com.formationds.om.webkit.rest.v07.snapshot.CreateSnapshot;
import com.formationds.om.webkit.rest.v07.snapshot.CreateSnapshotPolicy;
import com.formationds.om.webkit.rest.v07.snapshot.DeleteSnapshot;
import com.formationds.om.webkit.rest.v07.snapshot.DeleteSnapshotPolicy;
import com.formationds.om.webkit.rest.v07.snapshot.DetachSnapshotPolicyIdToVolumeId;
import com.formationds.om.webkit.rest.v07.snapshot.EditSnapshotPolicy;
import com.formationds.om.webkit.rest.v07.snapshot.GetSnapshot;
import com.formationds.om.webkit.rest.v07.snapshot.ListSnapshotPolicies;
import com.formationds.om.webkit.rest.v07.snapshot.ListSnapshotPoliciesForVolume;
import com.formationds.om.webkit.rest.v07.snapshot.ListSnapshotsByVolumeId;
import com.formationds.om.webkit.rest.v07.snapshot.ListVolumeIdsForSnapshotId;
import com.formationds.om.webkit.rest.v07.snapshot.RestoreSnapshot;
import com.formationds.om.webkit.rest.v07.stream.DeregisterStream;
import com.formationds.om.webkit.rest.v07.stream.ListStreams;
import com.formationds.om.webkit.rest.v07.stream.RegisterStream;
import com.formationds.om.webkit.rest.v07.tenants.AssignUserToTenant;
import com.formationds.om.webkit.rest.v07.tenants.CreateTenant;
import com.formationds.om.webkit.rest.v07.tenants.ListTenants;
import com.formationds.om.webkit.rest.v07.tenants.RevokeUserFromTenant;
import com.formationds.om.webkit.rest.v07.token.GrantToken;
import com.formationds.om.webkit.rest.v07.token.ReissueToken;
import com.formationds.om.webkit.rest.v07.token.ShowToken;
import com.formationds.om.webkit.rest.v07.users.CreateUser;
import com.formationds.om.webkit.rest.v07.users.CurrentUser;
import com.formationds.om.webkit.rest.v07.users.ListUsers;
import com.formationds.om.webkit.rest.v07.users.UpdatePassword;
import com.formationds.om.webkit.rest.v07.volumes.CloneVolume;
import com.formationds.om.webkit.rest.v07.volumes.CreateVolume;
import com.formationds.om.webkit.rest.v07.volumes.DeleteVolume;
import com.formationds.om.webkit.rest.v07.volumes.ListQosPresets;
import com.formationds.om.webkit.rest.v07.volumes.ListVolumes;
import com.formationds.om.webkit.rest.v07.volumes.SetVolumeQosParams;
import com.formationds.protocol.commonConstants;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import javax.crypto.SecretKey;

public class ApiDefinition extends AbstractApiDefinition
{

    private static final Logger logger = LogManager.getLogger( ApiDefinition.class );
	
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

	@Override
	public void configure() {
		// TODO Auto-generated method stub
		final ConfigurationApi configAPI = SingletonConfigAPI.instance().api();

        webApp.route( HttpMethod.POST, "/api/auth/token",
                      ( ) -> new GrantToken( configAPI,
                                             authenticator,
                                             authorizer,
                                             secretKey ) );
        webApp.route( HttpMethod.GET, "/api/auth/token",
                      ( ) -> new GrantToken( configAPI,
                                             authenticator,
                                             authorizer,
                                             secretKey ) );
        authenticate( HttpMethod.GET, "/api/auth/currentUser",
                      ( t ) -> new CurrentUser( authorizer,
                                                t ) );

        fdsAdminOnly( HttpMethod.GET, "/api/config/globaldomain",
                      ( t ) -> new ShowGlobalDomain() );

        // TODO: security model for statistics streams
        authenticate( HttpMethod.POST, "/api/config/streams",
                      ( t ) -> new RegisterStream( configAPI ) );
        authenticate( HttpMethod.GET, "/api/config/streams",
                      ( t ) -> new ListStreams( configAPI ) );
        authenticate( HttpMethod.PUT, "/api/config/streams",
                      ( t ) -> new DeregisterStream( configAPI ) );

        /*
         * provides snapshot RESTful API endpoints
         */
        snapshot( configAPI );

        /*
         * provides QoS RESTful API endpoints
         */
        qos( configAPI );

        /*
         * provides metrics RESTful API endpoints
         */
        metrics();

        /*
         * provides tenant RESTful API endpoints
         */
        tenants( secretKey, authorizer );

        // FIXME: DRY violation, definitely in com.formationds.commons.Fds, probably elsewhere.
        authenticate( HttpMethod.GET, "/api/config/volumes",
                      ( t ) -> new ListVolumes( authorizer,
                                                configAPI,
                                                t ) );
        authenticate( HttpMethod.POST, "/api/config/volumes",
                      ( t ) -> new CreateVolume( authorizer,
                                                 configAPI,
                                                 t ) );
        authenticate( HttpMethod.POST,
                      "/api/config/volumes/clone/:volumeId/:cloneVolumeName/:timelineTime",
                      ( t ) -> new CloneVolume( configAPI ) );
        authenticate( HttpMethod.DELETE, "/api/config/volumes/:name",
                      ( t ) -> new DeleteVolume( authorizer, configAPI,
                                                 t ) );
        authenticate( HttpMethod.PUT, "/api/config/volumes/:uuid",
                      ( t ) -> new SetVolumeQosParams(
                          SingletonConfigAPI.instance()
                                            .api(),
                          authorizer,
                          t ) );
        
        authenticate( HttpMethod.GET, "/api/config/volumes/presets/qos",
        			  ( t ) -> new ListQosPresets() );
        
        authenticate( HttpMethod.GET, "/api/config/volumes/presets/timeline",
  			  		  ( t ) -> new ListTimelinePresets() );        

        fdsAdminOnly( HttpMethod.GET, "/api/system/token/:userid",
                      ( t ) -> new ShowToken( configAPI,
                                              secretKey ) );
        fdsAdminOnly( HttpMethod.POST, "/api/system/token/:userid",
                      ( t ) -> new ReissueToken( configAPI,
                                                 secretKey ) );
        fdsAdminOnly( HttpMethod.POST, "/api/system/users/:login/:password",
                      ( t ) -> new CreateUser( configAPI,
                                               secretKey ) );
        authenticate( HttpMethod.PUT, "/api/system/users/:userid/:password",
                      ( t ) -> new UpdatePassword( t,
                                                   configAPI,
                                                   secretKey,
                                                   authorizer ) );
        fdsAdminOnly( HttpMethod.GET, "/api/system/users",
                      ( t ) -> new ListUsers( configAPI,
                                              secretKey ) );

        /*
         * provide events RESTful API endpoints
         */
        events();

        /*
         * provide platform RESTful API endpoints
         */
        platform();

        /*
         * Provide Local Domain RESTful API endpoints
         */
        localDomain();

        /*
         * Provides System Capabilities API endpoints
         */
        capability();
	}
	
	  private void capability() {

	        logger.trace( "registering system capabilities endpoints" );
	        /**
	         * Sprint 0.7.4 FS-1364 SSD Only Support
	         * 03/24/2015 10:00:00 AM
	         */
	        authenticate( HttpMethod.GET,
	                      "/api/config/system/capabilities",
	                      ( t ) -> new SystemCapabilities(
	                          SingletonConfiguration.instance()
	                                                .getConfig()
	                                                .getPlatformConfig() ) );
	        logger.trace( "registered system capabilities endpoints" );
	    }
	    private void platform( ) {

	        final ConfigurationApi configAPI = SingletonConfigAPI.instance().api();

	        logger.trace( "registering platform endpoints" );

	        fdsAdminOnly( HttpMethod.GET, "/api/config/nodes",
	                      ( t ) -> new ListNodes( configAPI ) );
	        fdsAdminOnly( HttpMethod.POST, "/api/config/nodes/:node_uuid/:domain_id",
	                      ( t ) -> new AddNode( configAPI ) );
	        fdsAdminOnly( HttpMethod.DELETE, "/api/config/nodes/:node_uuid",
	                      ( t ) -> new RemoveNode( configAPI ) );

	        fdsAdminOnly( HttpMethod.PUT, "/api/config/nodes/:node_uuid",
	        			  ( t ) -> new MutateNode( configAPI ) );
	        
	        fdsAdminOnly( HttpMethod.POST, "/api/config/nodes/:node_uuid/services",
	        			  ( t ) -> new AddService( configAPI ) );
	        
	        fdsAdminOnly( HttpMethod.DELETE, "/api/config/nodes/:node_uuid/services/:service_uuid", 
	        			  ( t ) -> new RemoveService( configAPI ) );
	        
	        fdsAdminOnly( HttpMethod.PUT, "/api/config/nodes/:node_uuid/services/:service_uuid",
	        			  ( t ) -> new MutateService( configAPI ) );
	        
	        logger.trace( "registered platform endpoints" );

	    }

	    private void localDomain( ) {

	        final ConfigurationApi configAPI = SingletonConfigAPI.instance().api();

	        logger.trace( "Registering Local Domain endpoints." );
	        
	        fdsAdminOnly( HttpMethod.POST,
	                      "/local_domains/:local_domain",
	                      ( t ) -> new PostLocalDomain( authorizer,
	                                                    configAPI,
	                                                    t ) );
	        fdsAdminOnly( HttpMethod.GET,
	                      "/local_domains",
	                      ( t ) -> new GetLocalDomains( authorizer,
	                                                    configAPI,
	                                                    t ) );
	        fdsAdminOnly( HttpMethod.PUT,
	                      "/local_domains/:local_domain",
	                      ( t ) -> new PutLocalDomain( authorizer,
	                                                   configAPI,
	                                                   t ) );
	        fdsAdminOnly( HttpMethod.PUT,
	                      "/local_domains/:local_domain/throttle",
	                      ( t ) -> new PutThrottle( authorizer,
	                                                configAPI,
	                                                t ) );
	        fdsAdminOnly( HttpMethod.PUT,
	                      "/local_domains/:local_domain/scavenger",
	                      ( t ) -> new PutScavenger( authorizer,
	                                                 configAPI,
	                                                 t ) );
	        fdsAdminOnly( HttpMethod.DELETE,
	                      "/local_domains/:local_domain",
	                      ( t ) -> new DeleteLocalDomain( authorizer,
	                                                      configAPI,
	                                                      t ) );

	        fdsAdminOnly( HttpMethod.GET,
	                      "/local_domains/:local_domain/services",
	                      ( t ) -> new GetLocalDomainServices( authorizer,
	                                                           configAPI,
	                                                           t ) );
	        fdsAdminOnly( HttpMethod.PUT,
	                      "/local_domains/:local_domain/services",
	                      ( t ) -> new PutLocalDomainServices( authorizer,
	                                                           configAPI,
	                                                           t ) );

	        logger.trace( "Registered Local Domain endpoints" );

	    }

	    private void metrics( ) {
	        logger.trace( "registering metrics endpoints" );
	        metricsGets();
	        metricsPost();
	        logger.trace( "registered metrics endpoints" );
	    }

	    private void metricsGets( ) {
	        authenticate( HttpMethod.PUT, "/api/stats/volumes",
	                      ( t ) -> new QueryMetrics( authorizer, t ) );
	        
	        authenticate( HttpMethod.PUT, "/api/stats/volumes/firebreak",
	        			( t ) -> new QueryFirebreak( authorizer, t ) );
	        
	        authenticate( HttpMethod.GET,  "/api/systemhealth",
	        		( t ) -> new SystemHealthStatus(
	        				SingletonConfigAPI.instance().api(),
	        				authorizer,
	        				t ) );
	    }

	    private void tenants( SecretKey secretKey, Authorizer authorizer ) {
	        //TODO: Add feature toggle

	        fdsAdminOnly( HttpMethod.POST, "/api/system/tenants/:tenant",
	                      ( t ) -> new CreateTenant(
	                          SingletonConfigAPI.instance()
	                                            .api(), secretKey ) );
	        fdsAdminOnly( HttpMethod.GET, "/api/system/tenants",
	                      ( t ) -> new ListTenants(
	                          SingletonConfigAPI.instance()
	                                            .api(), secretKey ) );
	        fdsAdminOnly( HttpMethod.PUT, "/api/system/tenants/:tenantid/:userid",
	                      ( t ) -> new AssignUserToTenant(
	                          SingletonConfigAPI.instance()
	                                            .api(), secretKey ) );
	        fdsAdminOnly( HttpMethod.DELETE,
	                      "/api/system/tenants/:tenantid/:userid",
	                      ( t ) -> new RevokeUserFromTenant(
	                          SingletonConfigAPI.instance()
	                                            .api(), secretKey ) );
	    }

	    private void metricsPost( ) {
	        webApp.route( HttpMethod.POST, "/api/stats",
	                      ( ) -> new IngestVolumeStats(
	                          SingletonConfigAPI.instance()
	                                            .api() ) );
	    }

	    private void snapshot( final ConfigurationApi config ) {
	        /**
	         * logical grouping for each HTTP method.
	         *
	         * This will allow future additions to the snapshot API to be extended
	         * and quickly view to ensure that all API are added. Its very lightweight,
	         * but make it easy to follow and maintain.
	         */
	        logger.trace( "registering snapshot endpoints" );
	        snapshotGets( config );
	        snapshotDeletes( config );
	        snapshotPosts( config );
	        snapshotPuts( config );
	        logger.trace( "registered snapshot endpoints" );
	    }

	    private void snapshotPosts( final ConfigurationApi config ) {
	        // POST methods
	        authenticate( HttpMethod.POST, "/api/config/snapshot/policies",
	                      ( t ) -> new CreateSnapshotPolicy( config ) );
	        authenticate( HttpMethod.POST,
	                      "/api/config/volumes/:volumeId/snapshot",
	                      ( t ) -> new CreateSnapshot( config ) );
	        authenticate( HttpMethod.POST,
	                      "/api/config/snapshot/restore/:snapshotId/:volumeId",
	                      ( t ) -> new RestoreSnapshot( config ) );
	        authenticate( HttpMethod.POST,
	                      "/api/config/snapshot/clone/:snapshotId/:cloneVolumeName",
	                      ( t ) -> new CloneSnapshot( config ) );
	    }

	    private void snapshotPuts( final ConfigurationApi config ) {
	        //PUT methods
	        authenticate( HttpMethod.PUT,
	                      "/api/config/snapshot/policies/:policyId/attach/:volumeId",
	                      ( t ) -> new AttachSnapshotPolicyIdToVolumeId(
	                          config ) );
	        authenticate( HttpMethod.PUT,
	                      "/api/config/snapshot/policies/:policyId/detach/:volumeId",
	                      ( t ) -> new DetachSnapshotPolicyIdToVolumeId(
	                          config ) );
	        authenticate( HttpMethod.PUT, "/api/config/snapshot/policies",
	                      ( t ) -> new EditSnapshotPolicy( config ) );
	    }

	    private void snapshotGets( final ConfigurationApi config ) {
	        // GET methods
	        authenticate( HttpMethod.GET, "/api/config/snapshot/policies",
	                      ( t ) -> new ListSnapshotPolicies( config ) );
	        authenticate( HttpMethod.GET,
	                      "/api/config/volumes/:volumeId/snapshot/policies",
	                      ( t ) -> new ListSnapshotPoliciesForVolume( config ) );
	        authenticate( HttpMethod.GET,
	                      "/api/config/snapshots/policies/:policyId/volumes",
	                      ( t ) -> new ListVolumeIdsForSnapshotId( config ) );
	        authenticate( HttpMethod.GET,
	                      "/api/config/volumes/:volumeId/snapshots",
	                      ( t ) -> new ListSnapshotsByVolumeId( config ) );
	        
	        authenticate( HttpMethod.GET,
	        			  "/api/config/snapshots/:snapshotId",
	        			  ( t ) -> new GetSnapshot( config ) );
	    }

	    private void snapshotDeletes( final ConfigurationApi config ) {
	        // DELETE methods
	        authenticate( HttpMethod.DELETE,
	                      "/api/config/snapshot/policies/:policyId",
	                      ( t ) -> new DeleteSnapshotPolicy( config ) );
          authenticate( HttpMethod.DELETE,
                        "/api/config/snapshot/:snapshotName",
                        ( t ) -> new DeleteSnapshot( authorizer,
                                                     config,
                                                     t ) );

	    }

	    private void qos( final ConfigurationApi config ) {
	        /**
	         * Logical grouping for each HTTP method.
	         *
	         * This will allow future additions to the QoS API to be extended
	         * and quickly view to ensure that all API are added. Its very lightweight,
	         * but makes it easy to follow and maintain.
	         */
	        logger.trace( "registering QoS endpoints" );
	        qosPosts( config );
	        qosGets( config );
	        qosPuts( config );
	        qosDeletes( config );
	        logger.trace("registered QoS endpoints");
	    }

	    private void qosPosts( final ConfigurationApi config ) {
	        authenticate(HttpMethod.POST,
	                "/fds/config/" + commonConstants.CURRENT_XDI_VERSION + "/qos_policies",
	                (t) -> new PostQoSPolicy(config));
	    }

	    private void qosGets( final ConfigurationApi config ) {
	        authenticate( HttpMethod.GET,
	                      "/fds/config/" + commonConstants.CURRENT_XDI_VERSION + "/qos_policies",
	                      ( t ) -> new GetQoSPolicies( config ) );
	    }

	    private void qosPuts( final ConfigurationApi config ) {
	        authenticate( HttpMethod.PUT,
	                      "/fds/config/" + commonConstants.CURRENT_XDI_VERSION + "/qos_policies/:current_policy_name",
	                      ( t ) -> new PutQoSPolicy( config ) );
	    }

	    private void qosDeletes( final ConfigurationApi config ) {
	        authenticate( HttpMethod.DELETE,
	                      "/fds/config/" + commonConstants.CURRENT_XDI_VERSION + "/qos_policies/:policy_name",
	                      ( t ) -> new DeleteQoSPolicy( config ) );
	    }

	    private void events( ) {

	        logger.trace( "registering activities endpoints" );

	        // TODO: only the AM should be sending this event to us. How can we validate that?
	        webApp.route( HttpMethod.PUT, "/api/events/log/:event",
	                      IngestEvents::new );

	        authenticate( HttpMethod.PUT, "/api/config/events",
	                      ( t ) -> new QueryEvents() );

	        logger.trace( "registered activities endpoints" );
	    }	

	@Override
	protected WebApp getWebApp() {
		
		return this.webApp;
	}

	@Override
	protected Authorizer getAuthorizer() {
		
		return this.authorizer;
	}

	@Override
	protected Authenticator getAuthenticator() {

		return this.authenticator;
	}
}
