package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.text.MessageFormat;
import java.text.ParseException;
import java.util.Map;
import java.util.function.Supplier;

public class S3Authorizer implements Supplier<RequestHandler> {
    private Supplier<RequestHandler> supplier;

    public S3Authorizer(Supplier<RequestHandler> supplier) {
        this.supplier = supplier;
    }

    @Override
    public RequestHandler get() {
        return new RequestHandler() {
            @Override
            public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
                String header = request.getHeader("Authorization");
                TextResource failure = new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
                if (header == null) {
                    return failure;
                }

                try {
                    tryParse(header);
                    return supplier.get().handle(request, routeParameters);
                } catch (SecurityException e) {
                    return failure;
                }
            }
        };
    }

    private void tryParse(String header) {
        String pattern = "AWS {0}:{1}";
        Object[] parsed = new Object[0];
        try {
            parsed = new MessageFormat(pattern).parse(header);
        } catch (ParseException e) {
            throw new SecurityException("invalid credentials");
        }
        String principal = (String) parsed[0];
        String tokenValue = (String) parsed[1];
        AuthorizationToken token = new AuthorizationToken(Authenticator.KEY, tokenValue);
        if (!token.isValid()) {
            throw new SecurityException("invalid credentials");
        }

        if (!token.getName().equals(principal)) {
            throw new SecurityException("invalid credentials");
        }
    }
}
