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
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.ConfigurationServiceCache;
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
    ConfigurationServiceCache configCache = new ConfigurationServiceCache(clientFactory.remoteOmService("localhost", 9090));
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
         * provides feature -- snapshot RESTful API
         */
    snapshot(legacyConfigClient, configCache);

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
        new com.formationds.demo.Main().start(configuration.getDemoConfig());
      } catch (Exception e) {
        LOG.error("Couldn't start demo app", e);
      }
    }).start();

    int adminWebappPort = platformConfig.lookup("fds.om.admin_webapp_port").intValue();
    webApp.start(adminWebappPort);
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

  /**
   * @param legacyConfigClient the legacy client configuration
   */
  private void snapshot(final FDSP_ConfigPathReq.Iface legacyConfigClient,
                        final ConfigurationServiceCache config) {
    if (!FdsFeatureToggles.SNAPSHOT_ENDPOINT.isActive()) {
      return;
    }

    // configure the ical4j system properties
/*
  Currently these are the only snapshot calls that are available!
  As Of 09/18/2014 @ 11:10 AM MST

     i64 createSnapshotPolicy(1:snapshot.SnapshotPolicy policy)
     throws (1: ApiException e),

     list<snapshot.SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
     throws (1: ApiException e),

     void deleteSnapshotPolicy(1:i64 id)
     throws (1: ApiException e),

     void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
     throws (1: ApiException e),

     list<snapshot.SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
     throws (1: ApiException e),

     void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
     throws (1: ApiException e),

     list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
     throws (1: ApiException e),

     list<snapshot.Snapshot> listSnapshots(1:i64 volumeId)
     throws (1: ApiException e),

     void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
     throws (1: ApiException e),

     i64 cloneVolume(1:i64 volumeId, 2:i64 fdsp_PolicyInfoId, 3:string cloneVolumeName)
*/

    LOG.trace("registering snapshot restful api");
    // POST methods
    snapshotPost(legacyConfigClient, config);
    // GET methods
    snapshotGet(config);
    //PUT methods
    snapshotPut(config);
    // DELETE methods
    snapshotDelete(config);
    LOG.trace("registered snapshot restful api");
  }

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  private void snapshotDelete(final ConfigurationServiceCache config) {
    /*
     * void deleteSnapshotPolicy(1:i64 id)
     *   throws (1: ApiException e),
     */
    authenticate(HttpMethod.DELETE, "/api/config/snapshot/policies/:policyId",
            (t) -> new DeleteSnapshotPolicy(config));

    /*
     * TODO this call does not currently exists
     */
    authenticate(HttpMethod.DELETE,
            "/api/config/snapshot/:volumeId/:snapshotId",
            (t) -> new DeleteSnapshotForVolume(config));
  }

  /**
   * @param legacyConfigClient the legacy client configuration
   * @param config             the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  private void snapshotPost(final FDSP_ConfigPathReq.Iface legacyConfigClient,
                            final ConfigurationServiceCache config) {
    /*
     * i64 createSnapshotPolicy(1:snapshot.SnapshotPolicy policy)
     *    throws (1: ApiException e)
     */
    authenticate(HttpMethod.POST, "/api/config/snapshot/policies",
            (t) -> new CreateSnapshotPolicy(xdi,
                    legacyConfigClient,
                    t));
    /*
     * TODO this call does not currently exists
     */
    authenticate(HttpMethod.POST, "/api/config/volumes/:volumeId/snapshot",
            (t) -> new CreateSnapshot(config));
    /*
     * void restoreClone(1:i64 volumeId, 2:i64 snapshotId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.POST, "/api/snapshot/restore/:snapshotId/:volumeId",
            (t) -> new RestoreSnapshot(config));
    /*
     * i64 cloneVolume(1:i64 volumeId, 2:i64 fdsp_PolicyInfoId, 3:string cloneVolumeName)
     */
    authenticate(HttpMethod.POST, "/api/snapshot/clone/:snapshotId/:cloneVolumeName",
            (t) -> new CloneSnapshot(config));

    /*
     * TODO this call does not currently exists
     */
    authenticate(HttpMethod.POST, "/api/volume/snapshot",
            (t) -> new CreateSnapshot(config));
  }

  private void snapshotGet(final ConfigurationServiceCache config) {
    /*
     * list<snapshot.SnapshotPolicy> listSnapshotPolicies(1:i64 unused)
     *   throws (1: ApiException e),
     */
    authenticate(HttpMethod.GET, "/api/config/snapshot/policies",
            (t) -> new ListSnapshotPolicies(config));

    /*
     * list<snapshot.SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.GET, "/api/config/volumes/:volumeId/snapshot/policies",
            (t) -> new ListSnapshotPoliciesForAVolume(config));

    /*
     * list<i64> listVolumesForSnapshotPolicy(1:i64 policyId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.GET, "/api/config/snapshots/policies/:policyId/volumes",
            (t) -> new ListVolumeIdsForSnapshotId(config));

    /*
     * list<snapshot.SnapshotPolicy> listSnapshotPoliciesForVolume(1:i64 volumeId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.GET, "/api/config/volumes/:volumeId/snapshots",
            (t) -> new ListSnapshotsByVolumeId(config));
  }

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  private void snapshotPut(final ConfigurationServiceCache config) {
    /*
     * void attachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.PUT,
            "/api/config/snapshot/policies/:policyId/attach",
            (t) -> new AttachSnapshotPolicyIdToVolumeId(config));
    /*
     * void detachSnapshotPolicy(1:i64 volumeId, 2:i64 policyId)
     *    throws (1: ApiException e),
     */
    authenticate(HttpMethod.PUT,
            "/api/config/snapshot/policies/:policyId/detach",
            (t) -> new DetachSnapshotPolicyIdToVolumeId(config));
  }
}

