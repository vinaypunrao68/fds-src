package com.formationds.web.om;
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
import java.util.Optional;
import java.util.function.Function;
import java.util.function.Supplier;

public class Authorizer implements RequestHandler {
    public static final String FDS_TOKEN = "token";
    private Supplier<RequestHandler> supplier;
    private Function<String, Boolean> authorizer;

    public Authorizer(Supplier<RequestHandler> supplier, Function<String, Boolean> authorizer) {
        this.supplier = supplier;
        this.authorizer = authorizer;
    }


    @Override
    public Resource handle(Request request) throws Exception {
        Cookie[] cookies = request.getCookies() == null? new Cookie[0] : request.getCookies();
        Optional<Boolean> result = Arrays.stream(cookies)
                .filter(c -> FDS_TOKEN.equals(c.getName()))
                .findFirst()
                .map(c -> authorizer.apply(c.getValue()));
        if (result.isPresent() && result.get()) {
            return supplier.get().handle(request);
        } else {
            return new JsonResource(new JSONObject().put("message", "Invalid credentials"), HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
