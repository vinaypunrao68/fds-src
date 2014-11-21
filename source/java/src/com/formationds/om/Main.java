/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.commons.events.EventManager;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.rest.*;
import com.formationds.om.rest.events.IngestEvents;
import com.formationds.om.rest.events.QueryEvents;
import com.formationds.om.rest.metrics.IngestVolumeStats;
import com.formationds.om.rest.metrics.QueryMetrics;
import com.formationds.om.rest.snapshot.*;
import com.formationds.security.*;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.ConfigurationApi;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import org.apache.commons.codec.binary.Hex;
import org.apache.log4j.Logger;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.servlet.http.HttpServletResponse;
import java.util.function.Function;

public class Main {
    private static final Logger LOG = Logger.getLogger(Main.class);

    private WebApp webApp;
    private Configuration configuration;

    private Xdi xdi;
    private ConfigurationApi configCache;

    // key for managing the singleton EventManager.
    private final Object eventMgrKey = new Object();

    public static void main(String[] args) {
        try {
            new Main().start(args);
        } catch (Throwable t) {
            LOG.fatal("Error starting OM", t);
            System.out.println(t.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }

    public void start(String[] args) throws Exception {

        configuration = new Configuration("om-xdi", args);
        SingletonConfiguration.instance().setConfig(configuration);

        // TODO there needs to be a "global" configuration access point to replace this
        System.setProperty( "fds-root", SingletonConfiguration.instance()
                                                              .getConfig()
                                                              .getFdsRoot() );

        LOG.trace( "FDS-ROOT:: " + System.getProperty( "fds-root" ) );

        NativeOm.startOm(args);

        ParsedConfig platformConfig = configuration.getPlatformConfig();
        byte[] keyBytes = Hex.decodeHex(platformConfig.lookup("fds.aes_key")
                                                      .stringValue()
                                                      .toCharArray());
        SecretKey secretKey = new SecretKeySpec(keyBytes, "AES");

        // TODO: this is needed before bootstrapping the admin user but not sure if there is config required first.
        // alternatively, we could initialize it with an empty event notifier or disabled flag and not log the
        // initial first-time bootstrap of the admin user as an event.
        EventManager.INSTANCE.initEventNotifier(eventMgrKey, (e) -> {
            return SingletonRepositoryManager.instance().getEventRepository().save(e) != null;
        });
        // initialize the firebreak event listener (callback from repository persist)
        EventManager.INSTANCE.initEventListeners();

        XdiClientFactory clientFactory = new XdiClientFactory();
        configCache = new ConfigurationApi(clientFactory.remoteOmService("localhost", 9090));
        EnsureAdminUser.bootstrapAdminUser(configCache);

        SingletonConfigAPI.instance().api( configCache );

        AmService.Iface amService = clientFactory.remoteAmService("localhost", 9988);

        String omHost = "localhost";
        int omPort = platformConfig.defaultInt("fds.om.config_port", 8903);
        String webDir = platformConfig.defaultString("fds.om.web_dir", "../lib/admin-webapp");

        SingletonAmAPI.instance().api( amService );

        FDSP_ConfigPathReq.Iface legacyConfigClient = clientFactory.legacyConfig(omHost, omPort);
//    Deprecated part of the old stats
//    VolumeStatistics volumeStatistics = new VolumeStatistics( Duration.standardMinutes( 20 ) );

        boolean enforceAuthentication = platformConfig.defaultBoolean("fds.authentication", true);
        Authenticator authenticator = enforceAuthentication ? new FdsAuthenticator(configCache, secretKey) : new NullAuthenticator();
        Authorizer authorizer = enforceAuthentication ? new FdsAuthorizer(configCache) : new DumbAuthorizer();

        xdi = new Xdi(amService, configCache, authenticator, authorizer, legacyConfigClient);

        webApp = new WebApp(webDir);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(webDir));

        webApp.route(HttpMethod.POST, "/api/auth/token", () -> new GrantToken(xdi, secretKey));
        webApp.route(HttpMethod.GET, "/api/auth/token", () -> new GrantToken(xdi, secretKey));
        authenticate(HttpMethod.GET, "/api/auth/currentUser", (t) -> new CurrentUser(xdi, t));

        fdsAdminOnly(HttpMethod.GET, "/api/config/services", (t) -> new ListServices(legacyConfigClient), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/config/services/:node_uuid", (t) -> new ActivatePlatform(legacyConfigClient), authorizer);

        fdsAdminOnly(HttpMethod.GET, "/api/config/globaldomain", (t) -> new ShowGlobalDomain(), authorizer);
        fdsAdminOnly(HttpMethod.GET, "/api/config/domains", (t) -> new ListDomains(), authorizer);

        // TODO: security model for statistics streams
        authenticate(HttpMethod.POST, "/api/config/streams", (t) -> new RegisterStream(configCache));
        authenticate(HttpMethod.GET, "/api/config/streams", (t) -> new ListStreams(configCache));
        authenticate(HttpMethod.PUT, "/api/config/streams", (t) -> new DeregisterStream(configCache));

        /*
         * provides snapshot RESTful API endpoints
         */
        snapshot(configCache, legacyConfigClient, authorizer);

        /*
         * provides metrics RESTful API endpoints
         */
        metrics();
        
        /*
         * provides tenant RESTful API endpoints
         */
        tenants( secretKey, authorizer );

        authenticate(HttpMethod.GET, "/api/config/volumes", (t) -> new ListVolumes(xdi, amService, legacyConfigClient, t));
        authenticate(HttpMethod.POST, "/api/config/volumes",
                (t) -> new CreateVolume(xdi, legacyConfigClient, configCache, t));
        authenticate(HttpMethod.POST, "/api/config/volumes/clone/:volumeId/:cloneVolumeName", (t) -> new CloneVolume(configCache, legacyConfigClient));
        authenticate(HttpMethod.DELETE, "/api/config/volumes/:name", (t) -> new DeleteVolume(xdi, t));
        authenticate(HttpMethod.PUT, "/api/config/volumes/:uuid", (t) -> new SetVolumeQosParams(xdi, legacyConfigClient, configCache, authorizer, t));

        fdsAdminOnly(HttpMethod.GET, "/api/system/token/:userid", (t) -> new ShowToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/token/:userid", (t) -> new ReissueToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/users/:login/:password", (t) -> new CreateUser(configCache, secretKey), authorizer);
        authenticate(HttpMethod.PUT, "/api/system/users/:userid/:password", (t) -> new UpdatePassword(t, configCache, secretKey, authorizer));
        fdsAdminOnly(HttpMethod.GET, "/api/system/users", (t) -> new ListUsers(configCache, secretKey), authorizer);

        /*
         * provide events RESTful API endpoints
         */
        events();

        int httpPort = platformConfig.defaultInt("fds.om.http_port", 7777);
        int httpsPort = platformConfig.defaultInt("fds.om.https_port", 7443);
        webApp.start(new HttpConfiguration(httpPort), new HttpsConfiguration(httpsPort, configuration));
  }

  private void fdsAdminOnly( HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f, Authorizer authorizer ) {
    authenticate( method, route, ( t ) -> {
      try {
        if( authorizer.userFor( t )
                      .isIsFdsAdmin() ) {
          return f.apply( t );
        } else {
          return ( r, p ) -> new JsonResource( new JSONObject().put( "message", "Invalid permissions" ), HttpServletResponse.SC_UNAUTHORIZED );
        }
      } catch( SecurityException e ) {
        LOG.error( "Error authorizing request, userId = " + t.getUserId(), e );
        return ( r, p ) -> new JsonResource( new JSONObject().put( "message", "Invalid permissions" ), HttpServletResponse.SC_UNAUTHORIZED );
      }
    } );
  }

  private void authenticate( HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f ) {
    HttpErrorHandler eh = new HttpErrorHandler( new HttpAuthenticator( f, xdi.getAuthenticator() ) );
    webApp.route( method, route, () -> eh );
  }

  private void metrics() {
    if( !FdsFeatureToggles.STATISTICS_ENDPOINT.isActive() ) {
      return;
    }

    LOG.trace( "registering metrics endpoints" );
    metricsGets();
    metricsPost();
    LOG.trace( "registered metrics endpoints" );
  }

  private void metricsGets() {
    authenticate( HttpMethod.PUT, "/api/stats/volumes",
                  ( t ) -> new QueryMetrics() );
  }
  
  private void tenants( SecretKey secretKey, Authorizer authorizer ) {
	  //TODO: Add feature toggle
	  
      fdsAdminOnly(HttpMethod.POST, "/api/system/tenants/:tenant", (t) -> new CreateTenant(configCache, secretKey), authorizer);
      fdsAdminOnly(HttpMethod.GET, "/api/system/tenants", (t) -> new ListTenants(configCache, secretKey), authorizer);
      fdsAdminOnly(HttpMethod.PUT, "/api/system/tenants/:tenantid/:userid", (t) -> new AssignUserToTenant(configCache, secretKey), authorizer);
      fdsAdminOnly(HttpMethod.DELETE, "/api/system/tenants/:tenantid/:userid", (t) -> new RevokeUserFromTenant( configCache, secretKey ), authorizer );
  }

  private void metricsPost() {
    webApp.route( HttpMethod.POST, "/api/stats", ( ) -> new IngestVolumeStats( configCache ) );
  }

  private void snapshot( final ConfigurationApi config,
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
    LOG.trace( "registering snapshot endpoints" );
    snapshotGets( config, authorizer );
    snapshotDeletes( config, authorizer );
    snapshotPosts( config, legacyConfigPath, authorizer );
    snapshotPuts( config, authorizer );
    LOG.trace( "registered snapshot endpoints" );
  }

  private void snapshotPosts( final ConfigurationApi config,
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

  private void snapshotPuts( final ConfigurationApi config,
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

    private void snapshotGets(final ConfigurationApi config,
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

  private void snapshotDeletes( final ConfigurationApi config,
                                final Authorizer authorizer ) {
    // DELETE methods
      authenticate( HttpMethod.DELETE, "/api/config/snapshot/policies/:policyId",
                  ( t ) -> new DeleteSnapshotPolicy( config ) );

      /*
       * TODO this call does not currently exists, maybe it should for API completeness
       */
//    fdsAdminOnly( HttpMethod.DELETE, "/api/config/snapshot/:volumeId/:snapshotId",
//                  ( t ) -> new DeleteSnapshotForVolume(), authorizer );
    }

    private void events() {
        if( !FdsFeatureToggles.ACTIVITIES_ENDPOINT.isActive() ) {
            return;
        }

        LOG.trace( "registering activities endpoints" );

        // TODO: only the AM should be sending this event to us.  How can we validate that?
        webApp.route(HttpMethod.PUT, "/api/events/log/:event", () -> new IngestEvents());

        authenticate(HttpMethod.PUT, "/api/config/events", (t) -> new QueryEvents());

//        authenticate(HttpMethod.GET, "/api/events/range/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/paged/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/tenants/:tenantId/range/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/tenants/:tenantId/paged/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/volumes/:volumeId/range/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/volumes/:volumeId/paged/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/users/:userId/range/:start/:end", (t) -> new QueryEvents());
//        authenticate(HttpMethod.GET, "/api/events/users/:userId/paged/:start/:end", (t) -> new QueryEvents());
        LOG.trace( "registered activities endpoints" );
    }

}

