/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.helper.SingletonLegacyConfig;
import com.formationds.om.webkit.rest.*;
import com.formationds.om.webkit.rest.events.IngestEvents;
import com.formationds.om.webkit.rest.events.QueryEvents;
import com.formationds.om.webkit.rest.metrics.IngestVolumeStats;
import com.formationds.om.webkit.rest.metrics.QueryMetrics;
import com.formationds.om.webkit.rest.snapshot.AttachSnapshotPolicyIdToVolumeId;
import com.formationds.om.webkit.rest.snapshot.CloneSnapshot;
import com.formationds.om.webkit.rest.snapshot.CreateSnapshot;
import com.formationds.om.webkit.rest.snapshot.CreateSnapshotPolicy;
import com.formationds.om.webkit.rest.snapshot.DeleteSnapshotPolicy;
import com.formationds.om.webkit.rest.snapshot.DetachSnapshotPolicyIdToVolumeId;
import com.formationds.om.webkit.rest.snapshot.EditSnapshotPolicy;
import com.formationds.om.webkit.rest.snapshot.ListSnapshotPolicies;
import com.formationds.om.webkit.rest.snapshot.ListSnapshotPoliciesForVolume;
import com.formationds.om.webkit.rest.snapshot.ListSnapshotsByVolumeId;
import com.formationds.om.webkit.rest.snapshot.ListVolumeIdsForSnapshotId;
import com.formationds.om.webkit.rest.snapshot.RestoreSnapshot;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;
import javax.servlet.http.HttpServletResponse;
import java.util.function.BiFunction;
import java.util.function.Function;

/**
 * @author ptinius
 */
public class WebKitImpl {

    private static final Logger logger =
        LoggerFactory.getLogger( WebKitImpl.class );

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

    public void start( String[] args ) {

        webApp = new WebApp( webDir );

        webApp.route( HttpMethod.GET, "", ( ) -> new LandingPage( webDir ) );

        webApp.route( HttpMethod.POST, "/api/auth/token",
                      ( ) -> new GrantToken( SingletonConfigAPI.instance()
                                                         .api(),
                                             authenticator,
                                             secretKey ) );
        webApp.route( HttpMethod.GET, "/api/auth/token",
                      ( ) -> new GrantToken( SingletonConfigAPI.instance()
                                                         .api(),
                                             authenticator,
                                             secretKey ) );
        authenticate( HttpMethod.GET, "/api/auth/currentUser",
                      ( t ) -> new CurrentUser( SingletonConfigAPI.instance()
                                                            .api(),
                                                t ) );

        fdsAdminOnly( HttpMethod.GET, "/api/config/services",
                      ( t ) -> new ListServices(
                          SingletonLegacyConfig.instance()
                                               .api() ),
                      authorizer );
        fdsAdminOnly( HttpMethod.POST, "/api/config/services/:node_uuid",
                      ( t ) -> new ActivatePlatform(
                          SingletonLegacyConfig.instance()
                                               .api() ),
                      authorizer );

        fdsAdminOnly( HttpMethod.GET, "/api/config/globaldomain",
                      ( t ) -> new ShowGlobalDomain(), authorizer );
        fdsAdminOnly( HttpMethod.GET, "/api/config/domains",
                      ( t ) -> new ListDomains(), authorizer );

        // TODO: security model for statistics streams
        authenticate( HttpMethod.POST, "/api/config/streams",
                      ( t ) -> new RegisterStream( SingletonConfigAPI.instance().api() ) );
        authenticate( HttpMethod.GET, "/api/config/streams",
                      ( t ) -> new ListStreams( SingletonConfigAPI.instance().api() ) );
        authenticate( HttpMethod.PUT, "/api/config/streams",
                      ( t ) -> new DeregisterStream( SingletonConfigAPI.instance().api() ) );

        /*
         * provides snapshot RESTful API endpoints
         */
        snapshot( SingletonConfigAPI.instance()
                                    .api(),
                  SingletonLegacyConfig.instance()
                                       .api(),
                  authorizer );

        /*
         * provides metrics RESTful API endpoints
         */
        metrics();

        /*
         * provides tenant RESTful API endpoints
         */
        tenants( secretKey, authorizer );

        authenticate( HttpMethod.GET, "/api/config/volumes",
                      ( t ) -> new ListVolumes( authorizer,
                                                SingletonConfigAPI.instance().api(),
                                                SingletonAmAPI.instance()
                                                              .api(),
                                                SingletonLegacyConfig.instance()
                                                                     .api(),
                                                t ) );
        authenticate( HttpMethod.POST, "/api/config/volumes",
                      ( t ) -> new CreateVolume( authorizer,
                                                 SingletonLegacyConfig.instance()
                                                                      .api(),
                                                 SingletonConfigAPI.instance().api(), t ) );
        authenticate( HttpMethod.POST,
                      "/api/config/volumes/clone/:volumeId/:cloneVolumeName/:timelineTime",
                      ( t ) -> new CloneVolume( SingletonConfigAPI.instance().api(),
                                                SingletonLegacyConfig.instance()
                                                                     .api() ) );
        authenticate( HttpMethod.DELETE, "/api/config/volumes/:name",
                      ( t ) -> new DeleteVolume( authorizer, SingletonConfigAPI.instance()
                                                             .api(),
                                                 t ) );
        authenticate( HttpMethod.PUT, "/api/config/volumes/:uuid",
                      ( t ) -> new SetVolumeQosParams( SingletonLegacyConfig.instance()
                                                                            .api(),
                                                       SingletonConfigAPI.instance()
                                                                         .api(),
                                                       authorizer,
                                                       t ) );

        fdsAdminOnly( HttpMethod.GET, "/api/system/token/:userid",
                      ( t ) -> new ShowToken( SingletonConfigAPI.instance().api(), secretKey ),
                      authorizer );
        fdsAdminOnly( HttpMethod.POST, "/api/system/token/:userid",
                      ( t ) -> new ReissueToken( SingletonConfigAPI.instance().api(), secretKey ),
                      authorizer );
        fdsAdminOnly( HttpMethod.POST, "/api/system/users/:login/:password",
                      ( t ) -> new CreateUser( SingletonConfigAPI.instance().api(), secretKey ),
                      authorizer );
        authenticate( HttpMethod.PUT, "/api/system/users/:userid/:password",
                      ( t ) -> new UpdatePassword( t, SingletonConfigAPI.instance().api(), secretKey,
                                                   authorizer ) );
        fdsAdminOnly( HttpMethod.GET, "/api/system/users",
                      ( t ) -> new ListUsers( SingletonConfigAPI.instance().api(), secretKey ),
                      authorizer );

        /*
         * provide events RESTful API endpoints
         */
        events();

        webApp.start(
            new HttpConfiguration( httpPort ),
            new HttpsConfiguration( httpsPort,
                                    SingletonConfiguration.instance()
                                                          .getConfig() ) );


    }

    private void fdsAdminOnly( HttpMethod method,
                               String route,
                               Function<AuthenticationToken, RequestHandler> f,
                               Authorizer authorizer ) {
        authenticate( method, route, ( t ) -> {
            try {
                if( authorizer.userFor( t )
                              .isIsFdsAdmin() ) {
                    return f.apply( t );
                } else {
                    return ( r, p ) ->
                        new JsonResource( new JSONObject().put( "message",
                                                                "Invalid permissions" ),
                                          HttpServletResponse.SC_UNAUTHORIZED );
                }
            } catch( SecurityException e ) {
                logger.error( "Error authorizing request, userId = " + t.getUserId(), e );
                return ( r, p ) ->
                    new JsonResource( new JSONObject().put( "message",
                                                            "Invalid permissions" ),
                                      HttpServletResponse.SC_UNAUTHORIZED );
            }
        } );
    }

    private void authenticate( HttpMethod method,
                               String route,
                               Function<AuthenticationToken,
                                   RequestHandler> f ) {
        HttpErrorHandler eh =
            new HttpErrorHandler(
                new HttpAuthenticator( f,
                                       authenticator) );
        webApp.route( method, route, ( ) -> eh );
    }

    private void metrics() {
        if( !FdsFeatureToggles.STATISTICS_ENDPOINT.isActive() ) {
            return;
        }

        logger.trace( "registering metrics endpoints" );
        metricsGets();
        metricsPost();
        logger.trace( "registered metrics endpoints" );
    }

    private void metricsGets() {
        authenticate( HttpMethod.PUT, "/api/stats/volumes",
                      ( t ) -> new QueryMetrics( authorizer, t ) );
    }

    private void tenants( SecretKey secretKey, Authorizer authorizer ) {
        //TODO: Add feature toggle

        fdsAdminOnly( HttpMethod.POST, "/api/system/tenants/:tenant", (t) -> new CreateTenant(
            SingletonConfigAPI.instance().api(), secretKey), authorizer);
        fdsAdminOnly( HttpMethod.GET, "/api/system/tenants", (t) -> new ListTenants(
            SingletonConfigAPI.instance().api(), secretKey), authorizer);
        fdsAdminOnly( HttpMethod.PUT, "/api/system/tenants/:tenantid/:userid", (t) -> new AssignUserToTenant(
            SingletonConfigAPI.instance().api(), secretKey), authorizer);
        fdsAdminOnly( HttpMethod.DELETE, "/api/system/tenants/:tenantid/:userid", (t) -> new RevokeUserFromTenant( SingletonConfigAPI.instance().api(), secretKey ), authorizer );
    }

    private void metricsPost() {
        webApp.route( HttpMethod.POST, "/api/stats",
                      ( ) -> new IngestVolumeStats(
                          SingletonConfigAPI.instance().api() ) );
    }

    private void snapshot( final com.formationds.util.thrift.ConfigurationApi config,
                           final FDSP_ConfigPathReq.Iface legacyConfigPath,
                           Authorizer authorizer ) {
        if( !FdsFeatureToggles.SNAPSHOT_ENDPOINT.isActive() ) {
            return;
        }

        /**
         * logical grouping for each HTTP method.
         *
         * This will allow future additions to the snapshot API to be extended
         * and quickly view to ensure that all API are added. Its very lightweight,
         * but make it easy to follow and maintain.
         */
        logger.trace( "registering snapshot endpoints" );
        snapshotGets( config, authorizer );
        snapshotDeletes( config, authorizer );
        snapshotPosts( config, legacyConfigPath, authorizer );
        snapshotPuts( config, authorizer );
        logger.trace( "registered snapshot endpoints" );
    }

    private void snapshotPosts( final com.formationds.util.thrift.ConfigurationApi config,
                                final FDSP_ConfigPathReq.Iface legacyConfigPath,
                                final Authorizer authorizer ) {
        // POST methods
        authenticate( HttpMethod.POST, "/api/config/snapshot/policies",
                      ( t ) -> new CreateSnapshotPolicy( config ) );
        authenticate( HttpMethod.POST, "/api/config/volumes/:volumeId/snapshot",
                      ( t ) -> new CreateSnapshot( config ) );
        authenticate( HttpMethod.POST,
                      "/api/config/snapshot/restore/:snapshotId/:volumeId",
                      ( t ) -> new RestoreSnapshot( config ) );
        authenticate( HttpMethod.POST,
                      "/api/config/snapshot/clone/:snapshotId/:cloneVolumeName",
                      ( t ) -> new CloneSnapshot( config, legacyConfigPath ) );
    }

    private void snapshotPuts( final com.formationds.util.thrift.ConfigurationApi config,
                               final Authorizer authorizer ) {
        //PUT methods
        authenticate( HttpMethod.PUT,
                      "/api/config/snapshot/policies/:policyId/attach/:volumeId",
                      ( t ) -> new AttachSnapshotPolicyIdToVolumeId( config ) );
        authenticate( HttpMethod.PUT,
                      "/api/config/snapshot/policies/:policyId/detach/:volumeId",
                      ( t ) -> new DetachSnapshotPolicyIdToVolumeId( config ) );
        authenticate( HttpMethod.PUT, "/api/config/snapshot/policies",
                      ( t ) -> new EditSnapshotPolicy( config ) );
    }

    private void snapshotGets(final com.formationds.util.thrift.ConfigurationApi config,
                              final Authorizer authorizer) {
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
    }

    private void snapshotDeletes( final com.formationds.util.thrift.ConfigurationApi config,
                                  final Authorizer authorizer ) {
        // DELETE methods
        authenticate( HttpMethod.DELETE, "/api/config/snapshot/policies/:policyId",
                      ( t ) -> new DeleteSnapshotPolicy( config ) );

    }

    private void events() {

        if( !FdsFeatureToggles.ACTIVITIES_ENDPOINT.isActive() ) {
            return;
        }

        logger.trace( "registering activities endpoints" );

        // TODO: only the AM should be sending this event to us.  How can we validate that?
        webApp.route( HttpMethod.PUT, "/api/events/log/:event",
                      ( ) -> new IngestEvents() );

        authenticate( HttpMethod.PUT, "/api/config/events", (t) -> new QueryEvents());

        logger.trace( "registered activities endpoints" );
    }
}
