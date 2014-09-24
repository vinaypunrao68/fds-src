package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.plotter.DisplayVolumeStats;
import com.formationds.om.plotter.ListActiveVolumes;
import com.formationds.om.plotter.RegisterVolumeStats;
import com.formationds.om.plotter.VolumeStatistics;
import com.formationds.om.rest.*;
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
import org.joda.time.Duration;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.servlet.http.HttpServletResponse;
import java.util.function.Function;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    private static final Logger LOG = Logger.getLogger(Main.class);

    private WebApp webApp;
    private Configuration configuration;
    private Xdi xdi;

    public static void main(String[] args) throws Exception {
        new Main().start(args);
    }

    public void start(String[] args) throws Exception {
        configuration = new Configuration("om-xdi", args);
        NativeOm.startOm(args);

        ParsedConfig platformConfig = configuration.getPlatformConfig();
        byte[] keyBytes = Hex.decodeHex(platformConfig.lookup("fds.aes_key").stringValue().toCharArray());
        SecretKey secretKey = new SecretKeySpec(keyBytes, "AES");

        XdiClientFactory clientFactory = new XdiClientFactory();
        ConfigurationApi configCache = new ConfigurationApi(clientFactory.remoteOmService("localhost", 9090));
        new EnsureAdminUser(configCache).execute();
        AmService.Iface amService = clientFactory.remoteAmService("localhost", 9988);

        String omHost = "localhost";
        int omPort = platformConfig.lookup("fds.om.config_port").intValue();
        String webDir = platformConfig.lookup("fds.om.web_dir").stringValue();

        FDSP_ConfigPathReq.Iface legacyConfigClient = clientFactory.legacyConfig(omHost, omPort);
        VolumeStatistics volumeStatistics = new VolumeStatistics(Duration.standardMinutes(20));

        boolean enforceAuthentication = platformConfig.lookup("fds.authentication").booleanValue();
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

        authenticate(HttpMethod.GET, "/api/config/volumes", (t) -> new ListVolumes(xdi, amService, legacyConfigClient, t));
        authenticate(HttpMethod.POST, "/api/config/volumes", (t) -> new CreateVolume(xdi, legacyConfigClient, t));
        authenticate(HttpMethod.DELETE, "/api/config/volumes/:name", (t) -> new DeleteVolume(xdi, t));
        authenticate(HttpMethod.PUT, "/api/config/volumes/:uuid", (t) -> new SetVolumeQosParams(legacyConfigClient, configCache, amService, authorizer, t));

        fdsAdminOnly(HttpMethod.GET, "/api/config/globaldomain", (t) -> new ShowGlobalDomain(), authorizer);
        fdsAdminOnly(HttpMethod.GET, "/api/config/domains", (t) -> new ListDomains(), authorizer);

        // TODO: security model for statistics streams
        authenticate(HttpMethod.POST, "/api/config/streams", (t) -> new RegisterStream(configCache));
        authenticate(HttpMethod.GET, "/api/config/streams", (t) -> new ListStreams(configCache));

        /*
         * provides snapshot RESTful API
         */
        snapshot(configCache, authorizer);

        // Demo volume stats
        webApp.route(HttpMethod.GET, "/api/stats/volumes", () -> new ListActiveVolumes(volumeStatistics));
        webApp.route(HttpMethod.POST, "/api/stats", () -> new RegisterVolumeStats(volumeStatistics));
        webApp.route(HttpMethod.GET, "/api/stats/volumes/:volume", () -> new DisplayVolumeStats(volumeStatistics));

        fdsAdminOnly(HttpMethod.GET, "/api/system/token/:userid", (t) -> new ShowToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/token/:userid", (t) -> new ReissueToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/tenants/:tenant", (t) -> new CreateTenant(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.GET, "/api/system/tenants", (t) -> new ListTenants(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/users/:login/:password", (t) -> new CreateUser(configCache, secretKey), authorizer);
        authenticate(HttpMethod.PUT, "/api/system/users/:userid/:password", (t) -> new UpdatePassword(t, configCache, secretKey, authorizer));
        fdsAdminOnly(HttpMethod.GET, "/api/system/users", (t) -> new ListUsers(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.PUT, "/api/system/tenants/:tenantid/:userid", (t) -> new AssignUserToTenant(configCache, secretKey), authorizer);

        new Thread(() -> {
            try {
                //new com.formationds.demo.Main().start(configuration.getDemoConfig());
            } catch (Exception e) {
                LOG.error("Couldn't start demo app", e);
            }
        }).start();

        int httpPort = platformConfig.lookup("fds.om.http_port").intValue();
        int httpsPort = platformConfig.lookup("fds.om.https_port").intValue();

        webApp.start(new HttpConfiguration(httpPort), new HttpsConfiguration(httpsPort, configuration));
    }

    private void fdsAdminOnly(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f, Authorizer authorizer) {
        authenticate(method, route, (t) -> {
            try {
                if (authorizer.userFor(t).isIsFdsAdmin()) {
                    return f.apply(t);
                } else {
                    return (r, p) -> new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
                }
            } catch (SecurityException e) {
                LOG.error("Error authorizing request, userId = " + t.getUserId(), e);
                return (r, p) -> new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
            }
        });
    }

    private void authenticate(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        HttpErrorHandler eh = new HttpErrorHandler(new HttpAuthenticator(f, xdi.getAuthenticator()));
        webApp.route(method, route, () -> eh);
    }

  private void snapshot( final ConfigurationApi config, Authorizer authorizer ) {
    if( !FdsFeatureToggles.SNAPSHOT_ENDPOINT.isActive() ) {
      return;
    }

    /**
     * logical grouping for each HTTP method.
     *
     * This will allow future additions to the snapshot API to be extended
     * and quickly view to ensure that all API are added. If very light weigh
     */
    LOG.trace( "registering snapshot restful api" );
    snapshotGets( config, authorizer );
    snapshotDeletes( config, authorizer );
    snapshotPosts( config, authorizer );
    snapshotPuts( config, authorizer );
    LOG.trace( "registered snapshot restful api" );
  }

  private void snapshotPosts( final ConfigurationApi config, Authorizer authorizer ) {
    // POST methods
    fdsAdminOnly( HttpMethod.POST, "/api/config/snapshot/policies",
                  ( t ) -> new CreateSnapshotPolicy( config ), authorizer );
    /*
     * TODO this call does not currently exists
     */
//    fdsAdminOnly( HttpMethod.POST, "/api/config/volumes/:volumeId/snapshot",
//                  ( t ) -> new CreateSnapshot(), authorizer );

    fdsAdminOnly( HttpMethod.POST, "/api/snapshot/restore/:snapshotId/:volumeId",
                  ( t ) -> new RestoreSnapshot( config ), authorizer );
    fdsAdminOnly( HttpMethod.POST, "/api/snapshot/clone/:snapshotId/:cloneVolumeName",
                  ( t ) -> new CloneSnapshot( config ), authorizer );
    fdsAdminOnly( HttpMethod.POST, "/api/volume/snapshot",
                  ( t ) -> new CreateSnapshot(), authorizer );
  }

  private void snapshotPuts( final ConfigurationApi config, Authorizer authorizer ) {
    //PUT methods
    fdsAdminOnly( HttpMethod.PUT,
                  "/api/config/snapshot/policies/:policyId/attach/:volumeId",
                  ( t ) -> new AttachSnapshotPolicyIdToVolumeId( config ),
                  authorizer );
    fdsAdminOnly( HttpMethod.PUT,
                  "/api/config/snapshot/policies/:policyId/detach/:volumeId",
                  ( t ) -> new DetachSnapshotPolicyIdToVolumeId( config ),
                  authorizer );
  }

  private void snapshotGets( final ConfigurationApi config, Authorizer authorizer ) {
    // GET methods
    fdsAdminOnly( HttpMethod.GET, "/api/config/snapshot/policies",
                  ( t ) -> new ListSnapshotPolicies( config ), authorizer );
    fdsAdminOnly( HttpMethod.GET, "/api/config/volumes/:volumeId/snapshot/policies",
                  ( t ) -> new ListSnapshotPoliciesForVolume( config ), authorizer );
    fdsAdminOnly( HttpMethod.GET, "/api/config/snapshots/policies/:policyId/volumes",
                  ( t ) -> new ListVolumeIdsForSnapshotId( config ), authorizer );
    fdsAdminOnly( HttpMethod.GET, "/api/config/volumes/:volumeId/snapshots",
                  ( t ) -> new ListSnapshotsByVolumeId( config ), authorizer );
  }

  private void snapshotDeletes( final ConfigurationApi config, Authorizer authorizer ) {
    // DELETE methods
    fdsAdminOnly( HttpMethod.DELETE, "/api/config/snapshot/policies/:policyId",
                  ( t ) -> new DeleteSnapshotPolicy( config ), authorizer );

      /*
       * TODO this call does not currently exists
       */
//    fdsAdminOnly( HttpMethod.DELETE, "/api/config/snapshot/:volumeId/:snapshotId",
//                  ( t ) -> new DeleteSnapshotForVolume(), authorizer );
  }
}

