package com.formationds.web.toolkit;

import com.formationds.web.toolkit.route.LexicalTrie;
import com.formationds.web.toolkit.route.QueryResult;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.util.MultiMap;

import java.util.function.Supplier;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class RouteFinder {
    private LexicalTrie<Supplier<RequestHandler>> map;

    public RouteFinder() {
        map = LexicalTrie.newTrie();
    }

    public void route(HttpMethod method, String name, Supplier<RequestHandler> handler) {
        name = method.toString() + name.replaceAll("^/", "");
        map.put(name, handler);
    }

    public Route resolve(Request request) {
        String path = request.getMethod().toString() + request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");
        QueryResult<Supplier<RequestHandler>> result = map.find(path);
        if (result.found()) {
            MultiMap<String> parameters = request.getParameters() == null ? new MultiMap<>() : request.getParameters();
            result.getMatches().forEach((k, v) -> parameters.add(k, v));
            request.setParameters(parameters);
            return new Route(request, result.getValue());
        }

        return new Route(request, () -> new FourOhFour());
    }
}
