package com.formationds.web.toolkit;

import org.eclipse.jetty.http.HttpURI;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.util.MultiMap;
import org.junit.Test;

import java.util.Map;
import java.util.Optional;

import static org.junit.Assert.*;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinderTest {
    @Test
    public void testWithWildcards() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.GET, "/foo/:bar", () -> new Foo());
        routeFinder.route(HttpMethod.GET, "/foo/:bar/hello", () -> new Bar());
        routeFinder.route(HttpMethod.GET, "/foo/:bar/hello/:panda", () -> new Foo());

        Route route = resolve(HttpMethod.GET, routeFinder, "/foo/andy").get();
        assertEquals("andy", route.getAttributes().get("bar"));

        route = resolve(HttpMethod.GET, routeFinder, "/foo/andy/hello").get();
        assertEquals("andy", route.getAttributes().get("bar"));

        route = resolve(HttpMethod.GET, routeFinder, "/foo/andy/hello////somethingelse").get();
        assertEquals("andy", route.getAttributes().get("bar"));
        assertEquals("somethingelse", route.getAttributes().get("panda"));
    }

    @Test
    public void testResolve() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.GET, "/hello/foo", () -> new Foo());
        routeFinder.route(HttpMethod.POST, "/hello/bar", () -> new Bar());

        assertTrue(resolve(HttpMethod.GET, routeFinder, "/hello/foo").isPresent());
        assertFalse(resolve(HttpMethod.POST, routeFinder, "/hello/foo/").isPresent());

        assertTrue(resolve(HttpMethod.POST, routeFinder, "/hello/bar").isPresent());
        assertFalse(resolve(HttpMethod.GET, routeFinder, "/hello/bar/").isPresent());

        assertFalse(resolve(HttpMethod.GET, routeFinder, "/poop").isPresent());
        assertFalse(resolve(HttpMethod.POST, routeFinder, "/poop").isPresent());
    }

    @Test
    public void testExpandArgs() throws Exception {
        RouteFinder routeFinder = new RouteFinder();
        routeFinder.route(HttpMethod.GET, "/a/:b/c/:d", () -> new Foo());

        Map<String, String> attributes = resolve(HttpMethod.GET, routeFinder, "/a/b/c/d").get().getAttributes();
        assertEquals("b", attributes.get("b"));
        assertEquals("d", attributes.get("d"));
    }

    private Optional<Route> resolve(HttpMethod httpMethod, RouteFinder routeFinder, String q) {
        Request request = new MockRequest(httpMethod.toString(), q, new MultiMap<>());
        return routeFinder.resolve(request);
    }

    class MockRequest extends Request {
        private String httpMethod;
        private String requestUri;
        private MultiMap<String> parameters;

        MockRequest(String httpMethod, String requestUri, MultiMap<String> parameters) {
            super(null, null);
            this.httpMethod = httpMethod;
            this.requestUri = requestUri;
            this.parameters = parameters;
        }

        @Override
        public String getMethod() {
            return httpMethod;
        }

        @Override
        public String getRequestURI() {
            return requestUri;
        }

        @Override
        public HttpURI getUri() {
            return new HttpURI(requestUri);
        }

        @Override
        public void setQueryParameters(MultiMap<String> parameters) {
            this.parameters = parameters;
        }

        @Override
        public String getParameter(String name) {
            return (String) parameters.getValue(name, 0);
        }
    }
    class Foo extends TextResource implements RequestHandler {
        public Foo() {
            super("foo");
        }

        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            return new TextResource("foo");
        }
    }

    class Bar extends TextResource implements RequestHandler {
        public Bar() {
            super("foo");
        }

        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            return new TextResource("foo");
        }
    }
}
