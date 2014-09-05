package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.om.plotter.DisplayVolumeStats;
import com.formationds.om.plotter.ListActiveVolumes;
import com.formationds.om.plotter.RegisterVolumeStats;
import com.formationds.om.plotter.VolumeStatistics;
import com.formationds.om.rest.*;
import com.formationds.security.*;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.ConfigurationServiceCache;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import org.apache.commons.codec.binary.Hex;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.joda.time.Duration;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.UUID;
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

        LegacyClientFactory legacyClientFactory = new LegacyClientFactory();
        FDSP_ConfigPathReq.Iface legacyConfigClient = legacyClientFactory.configPathClient(omHost, omPort);
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

        webApp.route(HttpMethod.GET, "/api/stats/volumes", () -> new ListActiveVolumes(volumeStatistics));
        webApp.route(HttpMethod.POST, "/api/stats", () -> new RegisterVolumeStats(volumeStatistics));
        webApp.route(HttpMethod.GET, "/api/stats/volumes/:volume", () -> new DisplayVolumeStats(volumeStatistics));


        fdsAdminOnly(HttpMethod.GET, "/api/system/token/:userid", (t) -> new ShowToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/token/:userid", (t) -> new ReissueToken(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/tenants/:tenant", (t) -> new CreateTenant(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.GET, "/api/system/tenants", (t) -> new ListTenants(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.POST, "/api/system/users/:login/:password", (t) -> new CreateUser(configCache, secretKey), authorizer);
        fdsAdminOnly(HttpMethod.PUT, "/api/system/users/:userid/:password", (t) -> new UpdatePassword(configCache, secretKey), authorizer);
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
            } catch (Exception e) {
                return (r, p) -> new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
            }
        });
    }

    private void authenticate(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        webApp.route(method, route, () -> new HttpAuthenticator(f, xdi.getAuthenticator()));
    }
}

class CreateAdminUser implements RequestHandler {
    private ConfigurationService.Iface config;

    public CreateAdminUser(ConfigurationService.Iface config) {
        this.config = config;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(routeParameters, "login");
        String password = requiredString(routeParameters, "password");

        long id = config.createUser(login, new HashedPassword().hash(password), UUID.randomUUID().toString(), true);
        return new JsonResource(new JSONObject().put("id", id));
    }
}
