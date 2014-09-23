package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.Xdi;

import javax.crypto.SecretKey;
import java.util.function.Function;

public class SwiftEndpoint {
    private Xdi xdi;
    private SecretKey secretKey;
    private WebApp webApp;

    public SwiftEndpoint(Xdi xdi, SecretKey secretKey) {
        this.xdi = xdi;
        this.secretKey = secretKey;
        webApp = new WebApp();
    }

    public void start(int httpPort) {
        webApp.route(HttpMethod.GET, "/v1/auth", () -> new SwiftFailureHandler((t) -> new GrantAuthenticationToken(xdi, secretKey)).apply(AuthenticationToken.ANONYMOUS));

        authenticate(HttpMethod.GET, "/v1/:account", (t) -> new ListContainers(xdi, t));
        authenticate(HttpMethod.GET, "/v1/:account/:container", (t) -> new GetContainer(xdi, t));
        authenticate(HttpMethod.PUT, "/v1/:account/:container", (t) -> new CreateContainer(xdi, t));
        authenticate(HttpMethod.DELETE, "/v1/:account/:container", (t) -> new DeleteContainer(xdi, t));
        authenticate(HttpMethod.PUT, "/v1/:account/:container/:object", (t) -> new CreateObject(xdi, t));
        authenticate(HttpMethod.GET, "/v1/:account/:container/:object", (t) -> new GetObject(xdi, t));
        authenticate(HttpMethod.DELETE, "/v1/:account/:container/:object", (t) -> new DeleteObject(xdi, t));

        webApp.start(new HttpConfiguration(httpPort, "0.0.0.0"));
    }

    private void authenticate(HttpMethod method, String route, Function<AuthenticationToken, RequestHandler> f) {
        Function<AuthenticationToken, RequestHandler> supplier = new SwiftFailureHandler(f);
        webApp.route(method, route, () -> new SwiftAuthenticator(supplier, xdi.getAuthenticator()));
    }
}
