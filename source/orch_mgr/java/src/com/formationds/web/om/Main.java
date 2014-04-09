package com.formationds.web.om;

import com.formationds.auth.AuthenticationToken;
import com.formationds.fdsp.ClientFactory;
import com.formationds.om.NativeApi;
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
    private static final int OM_PORT = 8903;
    private static String OM_HOST = "localhost";
    public static final String WEB_DIR = "../lib/admin-webapp/";

    private WebApp webApp;
    private Configuration configuration;

    public Main(String[] args) throws Exception {
        configuration = new Configuration(args);
        ClientFactory clientFactory = new ClientFactory();
        NativeApi.startOm(args);

        webApp = new WebApp(WEB_DIR);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(WEB_DIR));

        webApp.route(HttpMethod.POST, "/api/auth/token", () -> new IssueToken(KEY));
        webApp.route(HttpMethod.GET, "/api/auth/token", () -> new IssueToken(KEY));

        authorize(HttpMethod.GET, "/api/config/services", () -> new ListServices(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.POST, "/api/config/services/:node_uuid/:domain_id", () -> new ActivatePlatform(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        authorize(HttpMethod.GET, "/api/config/volumes", () -> new ListVolumes(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.POST, "/api/config/volumes/:name", () -> new CreateVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.POST, "/api/config/volume", () -> new FancyCreateVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.DELETE, "/api/config/volumes/:name", () -> new DeleteVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.PATCH, "/api/config/volume/:uuid", () -> new SetVolumeQosParams(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        authorize(HttpMethod.GET, "/api/config/globaldomain", ShowGlobalDomain::new);
        authorize(HttpMethod.GET, "/api/config/domains", ListDomains::new);
        authorize(HttpMethod.GET, "/api/config/volumeDefaults", () -> new ShowVolumeDefaults());
    }

    public void start() throws Exception {
        webApp.start(7777);
    }

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> factory) {
        if (configuration.enforceRestAuth()) {
            RequestHandler handler = new Authorizer(factory, s -> new AuthenticationToken(KEY, s).isValid());
            webApp.route(method, route, () -> handler);
        } else {
            webApp.route(method, route, factory);
        }
    }

    public static void main(String[] args) throws Exception {
            new Main(args).start();
    }
}

