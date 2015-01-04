package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.security.auth.login.LoginException;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.Arrays;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;

public class HttpAuthenticator implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(HttpAuthenticator.class);
    public static final String FDS_TOKEN = "token";
    private Function<AuthenticationToken, RequestHandler> f;
    private Authenticator authenticator;

    public HttpAuthenticator(Function<AuthenticationToken, RequestHandler> f, Authenticator authenticator) {
        this.f = f;
        this.authenticator = authenticator;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        AuthenticationToken token = null;

        // authenticate the request.
        Optional<AuthenticationToken> fromHeaders = parseHeaders(request);
        if (fromHeaders.isPresent()) {
            token = fromHeaders.get();
        }
        else {
            Cookie[] cookies = request.getCookies() == null ? new Cookie[0] : request.getCookies();
            Optional<Cookie> result = Arrays.stream(cookies)
                                            .filter(c -> FDS_TOKEN.equals(c.getName()))
                                            .findFirst();

            if (!result.isPresent()) {
                if (authenticator.allowAll()) {
                    token = AuthenticationToken.ANONYMOUS;
                } else {
                    return new JsonResource(new JSONObject().put("message", "Invalid credentials"), HttpServletResponse.SC_UNAUTHORIZED);
                }
            }  else {
                try {
                    token = authenticator.resolveToken(result.get().getValue());
                } catch (LoginException e) {
                    if (authenticator.allowAll()) {
                        LOG.error("Authentication error ignored - auth is disabled");
                        token = AuthenticationToken.ANONYMOUS;
                    } else {
                        LOG.error("Authentication error: " + e.getMessage());
                        if (LOG.isDebugEnabled())
                            LOG.debug("Authentication error: ", e);
                        return new JsonResource(new JSONObject().put("message", "Invalid credentials"), HttpServletResponse.SC_UNAUTHORIZED);
                    }
                }
            }
        }

        try {
            // Set the authentication in the thread local storage to allow access by the event manager
            AuthenticatedRequestContext.begin(token);
            return f.apply(token).handle(request, routeParameters);
        } finally {
            // remove the authentication token immediately upon completion of the request handling
            AuthenticatedRequestContext.complete();
        }
    }

    private Optional<AuthenticationToken> parseHeaders(Request request) {
        String headerValue = request.getHeader("FDS-Auth");
        if (StringUtils.isBlank(headerValue)) {
            return Optional.empty();
        }

        try {
            return Optional.of(authenticator.resolveToken(headerValue));
        } catch (Exception e) {
            return Optional.empty();
        }
    }
}
