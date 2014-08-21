package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.Arrays;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.function.Supplier;

public class HttpAuthenticator implements RequestHandler {
    public static final String FDS_TOKEN = "token";
    private Supplier<RequestHandler> supplier;
    private Function<String, Boolean> authprovider;

    public HttpAuthenticator(Supplier<RequestHandler> supplier, Function<String, Boolean> authprovider) {
        this.supplier = supplier;
        this.authprovider = authprovider;
    }


    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        Cookie[] cookies = request.getCookies() == null? new Cookie[0] : request.getCookies();
        Optional<Boolean> result = Arrays.stream(cookies)
                .filter(c -> FDS_TOKEN.equals(c.getName()))
                .findFirst()
                .map(c -> authprovider.apply(c.getValue()));
        if (result.isPresent() && result.get()) {
            return supplier.get().handle(request, routeParameters);
        } else {
            return new JsonResource(new JSONObject().put("message", "Invalid credentials"), HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
