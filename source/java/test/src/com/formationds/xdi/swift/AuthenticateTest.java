package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.UsageException;
import com.formationds.xdi.AuthenticationKey;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiCredentials;
import com.google.common.collect.Maps;
import com.sun.security.auth.UserPrincipal;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class AuthenticateTest {

    @Test(expected = UsageException.class)
    public void testHandle() throws Exception {
        Xdi xdi = mock(Xdi.class);
        Request request = mock(Request.class);
        Resource result = new Authenticate(xdi).handle(request, Maps.newHashMap());
    }

    @Test
    public void testInvalidCredentials() throws Exception {
        Xdi xdi = mock(Xdi.class);
        when(xdi.authenticate(anyString(), anyString())).thenThrow(LoginException.class);

        Request request = mock(Request.class);
        when(request.getHeader(Authenticate.X_AUTH_USER)).thenReturn("foo");
        when(request.getHeader(Authenticate.X_AUTH_KEY)).thenReturn("bar");

        Resource result = new Authenticate(xdi).handle(request, Maps.newHashMap());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, result.getHttpStatus());
    }

    @Test
    public void testValidCredentials() throws Exception {
        Xdi xdi = mock(Xdi.class);
        XdiCredentials credentials = new XdiCredentials(new UserPrincipal("foo"), new AuthenticationKey(new byte[] {1, 2}));
        when(xdi.authenticate(anyString(), anyString())).thenReturn(credentials);

        Request request = mock(Request.class);
        when(request.getHeader(Authenticate.X_AUTH_USER)).thenReturn("foo");
        when(request.getHeader(Authenticate.X_AUTH_KEY)).thenReturn("bar");

        Resource result = new Authenticate(xdi).handle(request, Maps.newHashMap());
        assertEquals(HttpServletResponse.SC_OK, result.getHttpStatus());
        assertEquals("AQI=", result.extraHeaders().get(Authenticate.X_AUTH_TOKEN).iterator().next());
    }
}
