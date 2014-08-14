package com.formationds.om;

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.om.rest.*;
import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.XdiClientFactory;
import org.apache.log4j.Logger;

import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    private static final Logger LOG = Logger.getLogger(Main.class);

    private WebApp webApp;
    private Configuration configuration;

    public static void main(String[] args) throws Exception {
        new Main().start(args);
    }

    public void start(String[] args) throws Exception {
        configuration = new Configuration("om-xdi", args);
        NativeOm.startOm(args);

        ParsedConfig omParsedConfig = configuration.getOmConfig();
        XdiClientFactory clientFactory = new XdiClientFactory();
        ConfigurationService.Iface configApi = clientFactory.remoteOmService("localhost", 9090);
        AmService.Iface amService = clientFactory.remoteAmService("localhost", 9988);

        String omHost = "localhost";
        int omPort = omParsedConfig.lookup("fds.om.config_port").intValue();
        String webDir = omParsedConfig.lookup("fds.om.web_dir").stringValue();

        LegacyClientFactory legacyClientFactory = new LegacyClientFactory();

        webApp = new WebApp(webDir);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(webDir));

        webApp.route(HttpMethod.POST, "/api/auth/token", () -> new IssueToken(Authenticator.KEY));
        webApp.route(HttpMethod.GET, "/api/auth/token", () -> new IssueToken(Authenticator.KEY));

        authorize(HttpMethod.GET, "/api/config/services", () -> new ListServices(legacyClientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.POST, "/api/config/services/:node_uuid", () -> new ActivatePlatform(legacyClientFactory.configPathClient(omHost, omPort)));

        authorize(HttpMethod.GET, "/api/config/volumes", () -> new ListVolumes(configApi, amService, legacyClientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.POST, "/api/config/volumes", () -> new CreateVolume(configApi, legacyClientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.DELETE, "/api/config/volumes/:name", () -> new DeleteVolume(legacyClientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.PUT, "/api/config/volumes/:uuid", () -> new SetVolumeQosParams(legacyClientFactory.configPathClient(omHost, omPort), configApi, amService));

        authorize(HttpMethod.GET, "/api/config/globaldomain", ShowGlobalDomain::new);
        authorize(HttpMethod.GET, "/api/config/domains", ListDomains::new);

        authorize(HttpMethod.POST, "/api/config/streams", () -> new RegisterStream(configApi));
        authorize(HttpMethod.GET, "/api/config/streams", () -> new ListStreams(configApi));

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

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> factory) {
        if (configuration.enforceRestAuth()) {
            RequestHandler handler = new Authorizer(factory, s -> new AuthorizationToken(Authenticator.KEY, s).isValid());
            webApp.route(method, route, () -> handler);
        } else {
            webApp.route(method, route, factory);
        }
    }
}

