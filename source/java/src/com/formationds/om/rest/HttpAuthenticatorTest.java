package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.Arrays;
import java.util.HashMap;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class HttpAuthenticatorTest {
    private final RequestHandler mockHandler;

    @Test
    public void testNoCookie() throws Exception {
        Request request = mock(Request.class);
        HttpAuthenticator httpAuthenticator = new HttpAuthenticator(() -> mockHandler, credentials -> false);
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, httpAuthenticator.handle(request, new HashMap<>()).getHttpStatus());
    }

    @Test
    public void testValidCookie() throws Exception {
        Request request = mock(Request.class);
        Cookie[] cookies = {new Cookie(HttpAuthenticator.FDS_TOKEN, "foo")};
        when(request.getCookies()).thenReturn(cookies);

        HttpAuthenticator httpAuthenticator = new HttpAuthenticator(() -> mockHandler, credentials -> {
            Cookie cookie = Arrays.stream(request.getCookies())
                    .filter(c -> HttpAuthenticator.FDS_TOKEN.equals(c.getName()))
                    .findFirst()
                    .get();
            assertEquals("foo", cookie.getValue());
            return true;
        });
        assertEquals(HttpServletResponse.SC_OK, httpAuthenticator.handle(request, new HashMap<>()).getHttpStatus());
    }

    @Test
    public void testInvalidCookie() throws Exception {
        Request request = mock(Request.class);
        Cookie[] cookies = {new Cookie(HttpAuthenticator.FDS_TOKEN, "foo")};
        when(request.getCookies()).thenReturn(cookies);

        HttpAuthenticator httpAuthenticator = new HttpAuthenticator(() -> mockHandler, credentials -> false);
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, httpAuthenticator.handle(request, new HashMap<>()).getHttpStatus());
    }

    public HttpAuthenticatorTest() {
        mockHandler = mock(RequestHandler.class);
        try {
            when(mockHandler.handle(any(Request.class), any(HashMap.class))).thenReturn(new TextResource("foo"));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
