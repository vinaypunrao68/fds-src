package com.formationds.om;

import com.formationds.auth.AuthenticationToken;
import com.formationds.fdsp.ClientFactory;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParserFacade;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    private static final SecretKey KEY = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");

    private WebApp webApp;
    private Configuration configuration;

    public static void main(String[] args) throws Exception {
        new Main(args).start();
    }

    public Main(String[] args) throws Exception {
        configuration = new Configuration(args);
        NativeOm.startOm(args);
    }

    public void start() throws Exception {
        ParserFacade omConfig = configuration.getOmConfig();
        String omHost = omConfig.lookup("fds.om.ip_address").stringValue();
        int omPort = omConfig.lookup("fds.om.config_port").intValue();
        String webDir = omConfig.lookup("fds.om.web_dir").stringValue();

        ClientFactory clientFactory = new ClientFactory();

        webApp = new WebApp(webDir);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(webDir));

        webApp.route(HttpMethod.POST, "/api/auth/token", () -> new IssueToken(KEY));
        webApp.route(HttpMethod.GET, "/api/auth/token", () -> new IssueToken(KEY));

        authorize(HttpMethod.GET, "/api/config/services", () -> new ListServices(clientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.POST, "/api/config/services/:node_uuid/:domain_id", () -> new ActivatePlatform(clientFactory.configPathClient(omHost, omPort)));

        authorize(HttpMethod.GET, "/api/config/volumes", () -> new ListVolumes(clientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.POST, "/api/config/volumes/:name", () -> new CreateVolume(clientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.POST, "/api/config/volume", () -> new FancyCreateVolume(clientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.DELETE, "/api/config/volumes/:name", () -> new DeleteVolume(clientFactory.configPathClient(omHost, omPort)));
        authorize(HttpMethod.PUT, "/api/config/volume/:uuid", () -> new SetVolumeQosParams(clientFactory.configPathClient(omHost, omPort)));

        authorize(HttpMethod.GET, "/api/config/globaldomain", ShowGlobalDomain::new);
        authorize(HttpMethod.GET, "/api/config/domains", ListDomains::new);
        authorize(HttpMethod.GET, "/api/config/volumeDefaults", () -> new ShowVolumeDefaults());

        int demoWebappPort = omConfig.lookup("fds.om.demo_webapp_port").intValue();
        new Thread(() -> new com.formationds.demo.Main().start(demoWebappPort)).start();

        int adminWebappPort = omConfig.lookup("fds.om.admin_webapp_port").intValue();
        webApp.start(adminWebappPort);
    }

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> factory) {
        if (configuration.enforceRestAuth()) {
            RequestHandler handler = new Authorizer(factory, s -> new AuthenticationToken(KEY, s).isValid());
            webApp.route(method, route, () -> handler);
        } else {
            webApp.route(method, route, factory);
        }
    }
}

