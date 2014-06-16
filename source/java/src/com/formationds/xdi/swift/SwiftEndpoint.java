package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

import java.util.function.Supplier;

public class SwiftEndpoint {
    private Xdi xdi;
    private boolean enforceAuth;
    private WebApp webApp;

    public SwiftEndpoint(Xdi xdi, boolean enforceAuth) {
        this.xdi = xdi;
        this.enforceAuth = enforceAuth;
        webApp = new WebApp();
    }

    public void start(int httpPort) {
        webApp.route(HttpMethod.GET, "/v1/auth", new SwiftFailureHandler(() -> new Authenticate(xdi)));

        authorize(HttpMethod.GET, "/v1/:account/:container", () -> new GetContainer(xdi));
        authorize(HttpMethod.PUT, "/v1/:account/:container", () -> new CreateContainer(xdi));
        authorize(HttpMethod.GET, "/v1/:account", () -> new ListContainers(xdi));
        authorize(HttpMethod.PUT, "/v1/:account/:container/:object", () -> new CreateObject(xdi));
        authorize(HttpMethod.GET, "/v1/:account/:container/:object", () -> new GetObject(xdi));
        authorize(HttpMethod.DELETE, "/v1/:account/:container/:object", () -> new DeleteObject(xdi));

        webApp.start(httpPort);
    }

    private void authorize(HttpMethod method, String route, Supplier<RequestHandler> s) {
        Supplier<RequestHandler> supplier = new SwiftFailureHandler(s);
        if (enforceAuth) {
            webApp.route(method, route, () -> new SwiftAuthorizer(supplier));
        } else {
            webApp.route(method, route, supplier);
        }
    }
}
