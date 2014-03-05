package com.formationds.web.om;

import com.formationds.om.NativeApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.log4j.PropertyConfigurator;

import java.io.File;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    public void main() throws Exception {
        PropertyConfigurator.configure(new File("/fds-root/etc/log4j-console.properties").toURL());
        new NativeApi();
        NativeApi.startOm();
        WebApp webApp = new WebApp();
        webApp.route(HttpMethod.get, "", () -> new LandingPage());
        webApp.route(HttpMethod.get, "nodes", () -> new ListNodes());
        webApp.start(4242);
    }

    public static void main(String[] args) throws Exception {
        new Main().main();
    }
}

