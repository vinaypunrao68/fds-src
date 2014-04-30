package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

public class SwiftEndpoint {
    private Xdi xdi;

    public SwiftEndpoint(Xdi xdi) {
        this.xdi = xdi;
    }

    public void start(int httpPort) {
        WebApp webApp = new WebApp();
        webApp.route(HttpMethod.GET, "/v1/:account/:container", () -> new GetContainer(xdi));

        webApp.start(httpPort);
    }
}
