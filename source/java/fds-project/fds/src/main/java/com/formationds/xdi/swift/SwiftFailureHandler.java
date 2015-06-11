package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.ApiException;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.*;

import javax.servlet.http.HttpServletResponse;
import java.util.function.Function;

public class SwiftFailureHandler implements Function<AuthenticationToken, RequestHandler> {
    private Function<AuthenticationToken, RequestHandler> f;

    public SwiftFailureHandler(Function<AuthenticationToken, RequestHandler> f) {
        this.f = f;
    }

    @Override
    public RequestHandler apply(AuthenticationToken t) {
        return (request, routeParameters) -> {
            try {
                return f.apply(t).handle(request, routeParameters);
            } catch (ApiException e) {
                return SwiftUtility.swiftResource(toSwiftResource(e));
            }
        };
    }

    private Resource toSwiftResource(ApiException e) {
        switch (e.getErrorCode()) {
            case MISSING_RESOURCE:
                return new FourOhFour();

            case BAD_REQUEST:
                return new TextResource(HttpServletResponse.SC_BAD_REQUEST, "");

            case INTERNAL_SERVER_ERROR:
                return new ErrorPage("Internal server error", e);

            case SERVICE_NOT_READY:
                return new TextResource(HttpServletResponse.SC_SERVICE_UNAVAILABLE, "FDS storage is not available yet");

            case RESOURCE_ALREADY_EXISTS:
                return new TextResource(HttpServletResponse.SC_BAD_REQUEST, "");

            case RESOURCE_NOT_EMPTY:
                return new TextResource(HttpServletResponse.SC_BAD_REQUEST, "Attempting to delete a non-empty volume");

            default:
                return new ErrorPage("Internal server error", e);
        }
    }
}
