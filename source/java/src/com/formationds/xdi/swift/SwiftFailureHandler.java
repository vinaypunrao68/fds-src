package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.web.toolkit.ErrorPage;
import com.formationds.web.toolkit.FourOhFour;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.TextResource;

import javax.servlet.http.HttpServletResponse;
import java.util.function.Supplier;

public class SwiftFailureHandler implements Supplier<RequestHandler> {
    private Supplier<RequestHandler> supplier;

    public SwiftFailureHandler(Supplier<RequestHandler> supplier) {
        this.supplier = supplier;
    }

    @Override
    public RequestHandler get() {
        return (request, routeParameters) -> {
            try {
                return supplier.get().handle(request, routeParameters);
            } catch (ApiException e) {
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
        };
    }
}
