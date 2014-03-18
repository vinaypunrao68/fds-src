package com.formationds.web.toolkit;

import org.eclipse.jetty.http.HttpMethod;
import org.eclipse.jetty.server.Request;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinder {
    private Map<Key, Supplier<RequestHandler>> map;

    public RouteFinder() {
        map = new HashMap<>();
    }

    public void route(HttpMethod method, String name, Supplier<RequestHandler> handler) {
        name = name.replaceAll("^/", "");
        map.put(new Key(method, name), handler);
    }

    public Route resolve(Request request) {
        String path = request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");
        HttpMethod method = HttpMethod.valueOf(request.getMethod().toUpperCase());
        Supplier<RequestHandler> supplier = map.computeIfAbsent(new Key(method, path), k -> () -> new FourOhFour());
        return new Route(request, supplier);
    }

    private class Key {
        private HttpMethod method;
        private String path;

        private Key(HttpMethod method, String path) {
            this.method = method;
            this.path = path;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Key key = (Key) o;

            if (method != key.method) return false;
            if (!path.equals(key.path)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = method.hashCode();
            result = 31 * result + path.hashCode();
            return result;
        }
    }
}
