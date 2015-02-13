package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.HttpMethod;

public class RouteSignature {
    private String method;
    private int pathElementCount;

    public RouteSignature(HttpMethod method, int pathElementCount) {
        this.method = method.toString().toLowerCase();
        this.pathElementCount = pathElementCount;
    }

    public RouteSignature(String method, int pathElementCount) {
        this.method = method.toLowerCase();
        this.pathElementCount = pathElementCount;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        @SuppressWarnings("unchecked")
        RouteSignature that = (RouteSignature) o;

        return pathElementCount == that.pathElementCount &&
               method.equals(that.method);
    }

    @Override
    public int hashCode() {
        int result = method.hashCode();
        result = 31 * result + pathElementCount;
        return result;
    }
}
