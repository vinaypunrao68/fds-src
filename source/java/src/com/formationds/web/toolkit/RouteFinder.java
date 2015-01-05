package com.formationds.web.toolkit;

import com.formationds.web.toolkit.route.LexicalTrie;
import com.formationds.web.toolkit.route.QueryResult;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.util.MultiMap;

import java.util.Optional;
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
        name = method.toString() + name
                .replaceAll("^/", "")
                .replaceAll("$/", "");
        map.put(name, handler);
    }

    public Optional<Route> resolve(Request request) {
        String path = request.getMethod() + request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "")
                .replaceAll("/$", "")
                .replaceAll("/+", "/");
        QueryResult<Supplier<RequestHandler>> result = map.find(path);



        //request.extractParameters();  // this consumes the stream and is EVIL
        if (result.found()) {
            if(request.getQueryParameters() == null) {
                MultiMap<String> qp = new MultiMap<>();
                request.getUri().decodeQueryTo(qp);
                request.setQueryParameters(qp);
            }
            return Optional.of(new Route(request, result.getMatches(), result.getValue()));
        }

        return Optional.empty();
    }
}
