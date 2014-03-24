package com.formationds.web.om;
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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class AuthorizerTest {
    private final RequestHandler mockHandler;

    @Test
    public void testNoCookie() throws Exception {
        Request request = mock(Request.class);
        Authorizer authorizer = new Authorizer(() -> mockHandler, credentials -> false);
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, authorizer.handle(request).getHttpStatus());
    }

    @Test
    public void testValidCookie() throws Exception {
        Request request = mock(Request.class);
        Cookie[] cookies = {new Cookie(Authorizer.FDS_TOKEN, "foo")};
        when(request.getCookies()).thenReturn(cookies);

        Authorizer authorizer = new Authorizer(() -> mockHandler, credentials -> {
            Cookie cookie = Arrays.stream(request.getCookies())
                    .filter(c -> Authorizer.FDS_TOKEN.equals(c.getName()))
                    .findFirst()
                    .get();
            assertEquals("foo", cookie.getValue());
            return true;
        });
        assertEquals(HttpServletResponse.SC_OK, authorizer.handle(request).getHttpStatus());
    }

    @Test
    public void testInvalidCookie() throws Exception {
        Request request = mock(Request.class);
        Cookie[] cookies = {new Cookie(Authorizer.FDS_TOKEN, "foo")};
        when(request.getCookies()).thenReturn(cookies);

        Authorizer authorizer = new Authorizer(() -> mockHandler, credentials -> false);
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, authorizer.handle(request).getHttpStatus());
    }

    public AuthorizerTest() {
        mockHandler = mock(RequestHandler.class);
        try {
            when(mockHandler.handle(any(Request.class))).thenReturn(new TextResource("foo"));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
