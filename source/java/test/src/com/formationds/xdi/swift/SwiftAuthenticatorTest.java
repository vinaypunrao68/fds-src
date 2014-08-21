package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.collect.Maps;
import com.sun.security.auth.UserPrincipal;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.servlet.http.HttpServletResponse;
import java.security.Principal;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class SwiftAuthenticatorTest {
    @Test
    public void testNoHeader() throws Exception {
        SwiftAuthenticator authorizer = new SwiftAuthenticator(() -> new StubHandler());

        Resource result = authorizer.handle(mock(Request.class), Maps.newHashMap());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    @Test
    public void testMalformedAuthHeader() throws Exception {
        SwiftAuthenticator authorizer = new SwiftAuthenticator(() -> new StubHandler());
        Request request = mock(Request.class);
        when(request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN)).thenReturn("poop");
        Resource result = authorizer.handle(request, Maps.newHashMap());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    @Test
    public void testWellFormedHeader() throws Exception {
        SecretKey secretKey = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");
        Principal principal = new UserPrincipal("admin");
        Request request = mock(Request.class);
        String token = new AuthenticationToken(secretKey, 1, "foo").signature();
        when(request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN)).thenReturn(token);
        SwiftAuthenticator authorizer = new SwiftAuthenticator(() -> new StubHandler());
        Resource result = authorizer.handle(request, Maps.newHashMap());
        assertEquals(HttpServletResponse.SC_OK, result.getHttpStatus());
    }

    class StubHandler implements RequestHandler {
        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            return new TextResource("");
        }
    }
}
