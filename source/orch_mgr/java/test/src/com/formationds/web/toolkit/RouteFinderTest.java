package com.formationds.web.toolkit;

import org.eclipse.jetty.server.Request;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinderTest {
    @Test
    public void testResolve() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.GET, "/hello/foo", () -> new Foo());
        routeFinder.route(HttpMethod.POST, "/hello/bar", () -> new Bar());

        Route route = resolve(HttpMethod.GET, routeFinder, "/hello/foo");
        assertEquals(200, route.getHandler().get().handle(route.getRequest()).getHttpStatus());

        route = resolve(HttpMethod.POST, routeFinder, "/hello/bar");
        assertEquals(200, route.getHandler().get().handle(route.getRequest()).getHttpStatus());

        route = resolve(HttpMethod.POST, routeFinder, "/hello/foo");
        assertEquals(404, route.getHandler().get().handle(route.getRequest()).getHttpStatus());

        route = resolve(HttpMethod.POST, routeFinder, "poop");
        assertEquals(404, route.getHandler().get().handle(route.getRequest()).getHttpStatus());
    }

    @Test
    public void testExpandArgs() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.GET, "/a/:color/b/:metal", () -> new Foo());

        Route route = resolve(HttpMethod.GET, routeFinder, "/a/blue/b/iron");
        assertEquals(200, route.getHandler().get().handle(route.getRequest()).getHttpStatus());
        assertEquals("blue", route.getRequest().getParameter("color"));
        assertEquals("iron", route.getRequest().getParameter("metal"));
    }

    private Route resolve(HttpMethod httpMethod, RouteFinder routeFinder, String q) {
        Request request = new Request();
        request.setMethod(httpMethod.toString());
        request.setRequestURI(q);
        return routeFinder.resolve(request);
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
