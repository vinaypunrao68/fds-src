package com.formationds.web.toolkit;

import org.apache.log4j.Logger;

import javax.servlet.http.HttpServletRequest;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinder {

    private Map<Key, Supplier<RequestHandler>> map;

    private Logger LOG = Logger.getLogger(RouteFinder.class);

    public RouteFinder() {
        map = new HashMap<>();
    }

    public void route(HttpMethod method, String name, Supplier<RequestHandler> handler) {
        map.put(new Key(method, name), handler);
    }

    public Supplier<RequestHandler> resolve(HttpServletRequest request) {
        String path = request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");
        HttpMethod method = HttpMethod.valueOf(request.getMethod().toLowerCase());
        return resolve(method, path);
    }

    public Supplier<RequestHandler> resolve(HttpMethod method, String path) {
        return map.computeIfAbsent(new Key(method, path), k -> () -> new FourOhFour());
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
