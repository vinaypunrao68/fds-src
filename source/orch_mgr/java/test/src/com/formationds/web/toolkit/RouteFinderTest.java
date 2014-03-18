package com.formationds.web.toolkit;

import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.servlet.http.HttpServletRequest;

import static org.junit.Assert.assertEquals;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinderTest {
    @Test
    public void testResolve() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.get, "/hello/foo", () -> new Foo());
        routeFinder.route(HttpMethod.post, "/hello/bar", () -> new Bar());
        assertEquals(200, routeFinder.resolve(HttpMethod.get, "/hello/foo").get().handle(null).getHttpStatus());
        assertEquals(404, routeFinder.resolve(HttpMethod.post, "/hello/foo").get().handle(null).getHttpStatus());
        assertEquals(200, routeFinder.resolve(HttpMethod.post, "/hello/bar").get().handle(null).getHttpStatus());
        assertEquals(404, routeFinder.resolve(HttpMethod.head, "/hello/bar").get().handle(null).getHttpStatus());
        assertEquals(404, routeFinder.resolve(HttpMethod.get, "/").get().handle(null).getHttpStatus());
    }

    class Foo extends TextResource implements RequestHandler {
        public Foo() {
            super("foo");
        }

        @Override
        public Resource handle(Request request) throws Exception {
            return new TextResource("foo");
        }
    }

    class Bar extends TextResource implements RequestHandler {
        public Bar() {
            super("foo");
        }

        @Override
        public Resource handle(Request request) throws Exception {
            return new TextResource("foo");
        }
    }
}
