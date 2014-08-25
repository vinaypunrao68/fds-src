package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.om.plotter.DisplayVolumeStats;
import com.formationds.om.plotter.ListActiveVolumes;
import com.formationds.om.plotter.RegisterVolumeStats;
import com.formationds.om.plotter.VolumeStatistics;
import com.formationds.om.rest.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.FdsAuthenticator;
import com.formationds.security.NullAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.CachingConfigurationService;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import org.apache.log4j.Logger;
import org.joda.time.Duration;

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

        ParsedConfig omParsedConfig = configuration.getOmConfig();
        XdiClientFactory clientFactory = new XdiClientFactory();
        CachingConfigurationService configApi = new CachingConfigurationService(clientFactory.remoteOmService("localhost", 9090));
        AmService.Iface amService = clientFactory.remoteAmService("localhost", 9988);

        String omHost = "localhost";
        int omPort = omParsedConfig.lookup("fds.om.config_port").intValue();
        String webDir = omParsedConfig.lookup("fds.om.web_dir").stringValue();

        LegacyClientFactory legacyClientFactory = new LegacyClientFactory();
        FDSP_ConfigPathReq.Iface legacyConfigClient = legacyClientFactory.configPathClient(omHost, omPort);
        VolumeStatistics volumeStatistics = new VolumeStatistics(Duration.standardMinutes(20));

        Authenticator authenticator = configuration.enforceRestAuth() ? new FdsAuthenticator(configApi) : new NullAuthenticator();

        xdi = new Xdi(amService, configApi, authenticator, legacyConfigClient);

        webApp = new WebApp(webDir);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(webDir));

        webApp.route(HttpMethod.POST, "/api/auth/token", () -> new IssueToken(xdi));
        webApp.route(HttpMethod.GET, "/api/auth/token", () -> new IssueToken(xdi));

        authorize(HttpMethod.GET, "/api/config/services", (t) -> new ListServices(legacyConfigClient));
        authorize(HttpMethod.POST, "/api/config/services/:node_uuid", (t) -> new ActivatePlatform(legacyConfigClient));

        authorize(HttpMethod.GET, "/api/config/volumes", (t) -> new ListVolumes(configApi, amService, legacyConfigClient));
        authorize(HttpMethod.POST, "/api/config/volumes", (t) -> new CreateVolume(configApi, legacyConfigClient));
        authorize(HttpMethod.DELETE, "/api/config/volumes/:name", (t) -> new DeleteVolume(legacyConfigClient));
        authorize(HttpMethod.PUT, "/api/config/volumes/:uuid", (t) -> new SetVolumeQosParams(legacyConfigClient, configApi, amService));

        authorize(HttpMethod.GET, "/api/config/globaldomain", (t) -> new ShowGlobalDomain());
        authorize(HttpMethod.GET, "/api/config/domains", (t) -> new ListDomains());

        authorize(HttpMethod.POST, "/api/config/streams", (t) -> new RegisterStream(configApi));
        authorize(HttpMethod.GET, "/api/config/streams", (t) -> new ListStreams(configApi));

        webApp.route(HttpMethod.GET, "/api/stats/volumes", () -> new ListActiveVolumes(volumeStatistics));
        webApp.route(HttpMethod.POST, "/api/stats", () -> new RegisterVolumeStats(volumeStatistics));
        webApp.route(HttpMethod.GET, "/api/stats/volumes/:volume", () -> new DisplayVolumeStats(volumeStatistics));

        new Thread(() -> {
            try {
                new com.formationds.demo.Main().start(configuration.getDemoConfig());
            } catch (Exception e) {
                LOG.error("Couldn't start demo app", e);
            }
        }).start();

        int adminWebappPort = omParsedConfig.lookup("fds.om.admin_webapp_port").intValue();
        webApp.start(adminWebappPort);
    }

    private void authorize(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        webApp.route(method, route, () -> new HttpAuthenticator(f, xdi.getAuthenticator()));
    }
}

