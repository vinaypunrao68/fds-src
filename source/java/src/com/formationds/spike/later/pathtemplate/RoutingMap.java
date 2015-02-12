package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.later.HttpPath;
import io.undertow.server.HttpServerExchange;

import java.util.*;

public class RoutingMap<T> {
    private Map<RouteSignature, List<KeyValuePair<HttpPath, T>>> signatureRoutingPathMap;
    private List<KeyValuePair<HttpPath, T>> pathlessRoutes;

    public RoutingMap() {
        signatureRoutingPathMap = new HashMap<>();
        pathlessRoutes = new ArrayList<>();
    }

    public RouteResult<T> get(HttpServerExchange request) {
        String path = request.getRequestPath();
        String[] segments = path.split("/");
        RouteSignature signature = new RouteSignature(request.getRequestMethod(), segments.length);
        List<KeyValuePair<HttpPath, T>> candidatePaths = signatureRoutingPathMap.getOrDefault(signature, Collections.emptyList());
        for(KeyValuePair<HttpPath, T> routingPath : candidatePaths) {
            HttpPath.MatchResult result = routingPath.getKey().matches(request);
            if(result.isMatch())
                return new RouteResult<>(routingPath.getValue(), result.getPathParameters(), true);
        }

        for(KeyValuePair<HttpPath,T> routingPath : pathlessRoutes) {
            HttpPath.MatchResult result = routingPath.getKey().matches(request);
            if(result.isMatch())
                return new RouteResult<>(routingPath.getValue(), Collections.emptyMap(), true);
        }

        return new RouteResult<>(null, null, false);
    }

    public void put(HttpPath path, T value) {
        Optional<RouteSignature> signature = path.routeSignature();
        KeyValuePair<HttpPath, T> entry = new KeyValuePair<>(path, value);

        if(!signature.isPresent())
            pathlessRoutes.add(entry);
        else {
            signatureRoutingPathMap.computeIfAbsent(signature.get(), s -> new ArrayList<>()).add(entry);
        }
    }
}

