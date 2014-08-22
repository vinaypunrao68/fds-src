package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.servlet.http.HttpServletResponse;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

public class S3AuthenticatorTest {

    //@Test
    public void testMissingHeader() throws Exception {
        RequestHandler success = mock(RequestHandler.class);
        Resource result = new S3Authenticator(() -> success).get().handle(mock(Request.class), new HashMap<>());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
        verifyZeroInteractions(success);
    }

    //@Test
    public void testAuthorizationSucceeds() throws Exception {
        Request request = mock(Request.class);
        String principalName = "joe";
        String signature = new AuthenticationToken(Authenticator.KEY, 42, "foo").signature();
        String headerValue = "AWS " + principalName + ":" + signature;
        when(request.getHeader("Authorization")).thenReturn(headerValue);
        Resource result = new S3Authenticator(() -> new Handler()).get().handle(request, new HashMap<>());
        assertEquals(HttpServletResponse.SC_OK, result.getHttpStatus());
    }

    //@Test
    public void testMalformedHeader() throws Exception {
        Request request = mock(Request.class);
        String headerValue = "poop";
        when(request.getHeader("Authorization")).thenReturn(headerValue);
        Resource result = new S3Authenticator(() -> new Handler()).get().handle(request, new HashMap<>());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    //@Test
    public void testMalformedToken() throws Exception {
        Request request = mock(Request.class);
        String headerValue = "AWS joe:poop";
        when(request.getHeader("Authorization")).thenReturn(headerValue);
        Resource result = new S3Authenticator(() -> new Handler()).get().handle(request, new HashMap<>());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    //@Test
    public void testMismatchedCredentials() throws Exception {
        Request request = mock(Request.class);
        String principalName = "joe";
        String signature = new AuthenticationToken(Authenticator.KEY, 42, "foo").signature();
        String headerValue = "AWS frank:" + signature;
        when(request.getHeader("Authorization")).thenReturn(headerValue);
        Resource result = new S3Authenticator(() -> new Handler()).get().handle(request, new HashMap<>());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    class Handler implements RequestHandler {
        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            return new TextResource("");
        }
    }

    @Test
    public void makeJUnitHappy() {

    }
}
