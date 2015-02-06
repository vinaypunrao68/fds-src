package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Map;

public class RouteResult<T> {
    private boolean routingSuccessful;
    private T result;
    private Map<String, String> routeParameters;

    RouteResult(T result, Map<String, String> routeParameters, boolean routingSuccessful) {
        this.result = result;
        this.routeParameters = routeParameters;
        this.routingSuccessful = routingSuccessful;
    }

    public T getResult() {
        return result;
    }

    public Map<String, String> getRouteParameters() {
        return routeParameters;
    }

    public boolean isRoutingSuccessful() {
        return routingSuccessful;
    }
}
