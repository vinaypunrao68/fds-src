package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

import java.util.function.Function;

public class SwiftEndpoint {
    private Xdi xdi;
    private WebApp webApp;

    public SwiftEndpoint(Xdi xdi) {
        this.xdi = xdi;
        webApp = new WebApp();
    }

    public void start(int httpPort) {
        webApp.route(HttpMethod.GET, "/v1/auth", () -> new SwiftFailureHandler((t) -> new GrantAuthenticationToken(xdi)).apply(AuthenticationToken.ANONYMOUS));

        authorize(HttpMethod.GET, "/v1/:account/:container", (t) -> new GetContainer(xdi));
        authorize(HttpMethod.PUT, "/v1/:account/:container", (t) -> new CreateContainer(xdi));
        authorize(HttpMethod.DELETE, "/v1/:account/:container", (t) -> new DeleteContainer(xdi));
        authorize(HttpMethod.GET, "/v1/:account", (t) -> new ListContainers(xdi));
        authorize(HttpMethod.PUT, "/v1/:account/:container/:object", (t) -> new CreateObject(xdi));
        authorize(HttpMethod.GET, "/v1/:account/:container/:object", (t) -> new GetObject(xdi));
        authorize(HttpMethod.DELETE, "/v1/:account/:container/:object", (t) -> new DeleteObject(xdi));

        webApp.start(httpPort);
    }

    private void authorize(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        Function<AuthenticationToken, RequestHandler> supplier = new SwiftFailureHandler(f);
        webApp.route(method, route, () -> new SwiftAuthenticator(supplier, xdi.getAuthenticator()));
    }
}
