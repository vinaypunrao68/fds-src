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
    private final Configuration configuration;

    public Main() throws Exception {
        configuration = new Configuration();
    }

    public void start(String[] args) throws Exception {
        ClientFactory clientFactory = new ClientFactory();
        NativeApi.startOm(args);

        webApp = new WebApp(WEB_DIR);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(WEB_DIR));

        webApp.route(HttpMethod.POST, "/auth/token", () -> new IssueToken(KEY));
        webApp.route(HttpMethod.GET, "/auth/token", () -> new IssueToken(KEY));

        authorize(HttpMethod.GET, "/config/services", () -> new ListServices(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        authorize(HttpMethod.GET, "/config/volumes", () -> new ListVolumes(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.POST, "/config/volumes/:name", () -> new CreateVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        authorize(HttpMethod.DELETE, "/config/volumes/:name", () -> new DeleteVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        authorize(HttpMethod.GET, "/config/globaldomain", ShowGlobalDomain::new);
        authorize(HttpMethod.GET, "/config/domains", ListDomains::new);

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
            new Main().start(args);
    }
}

