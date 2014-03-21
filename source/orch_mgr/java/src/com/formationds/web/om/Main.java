package com.formationds.web.om;

import com.formationds.fdsp.ClientFactory;
import com.formationds.om.NativeApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.log4j.Logger;

import java.lang.management.ManagementFactory;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);
    public static final int OM_PORT = 8903;
    private static String OM_HOST = "localhost";

    public void start(String[] args) throws Exception {
        ClientFactory clientFactory = new ClientFactory();
        new NativeApi();
        NativeApi.startOm(args);
        LOG.info("Process info: " + ManagementFactory.getRuntimeMXBean().getName());
        String webDir = "../lib/admin-webapp/";

        WebApp webApp = new WebApp(webDir);

        webApp.route(HttpMethod.GET, "", () -> new LandingPage(webDir));

        webApp.route(HttpMethod.POST, "/session/authenticate", new AuthenticateAction());
        webApp.route(HttpMethod.GET, "/config/services", () -> new ListServices(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        webApp.route(HttpMethod.GET, "/config/volumes", () -> new ListVolumes(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        webApp.route(HttpMethod.POST, "/config/volumes/:name", () -> new CreateVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));
        webApp.route(HttpMethod.DELETE, "/config/volumes/:name", () -> new DeleteVolume(clientFactory.configPathClient(OM_HOST, OM_PORT)));

        webApp.route(HttpMethod.GET, "/config/globaldomain", () -> new ShowGlobalDomain());
        webApp.route(HttpMethod.GET, "/config/domains", () -> new ListDomains());

        webApp.start(7777);
    }

    public static void main(String[] args) throws Exception {
            new Main().start(args);
    }
}

