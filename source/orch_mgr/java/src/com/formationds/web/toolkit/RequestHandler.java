package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.util.Map;
import java.util.Optional;

public interface RequestHandler {
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception;

    public default String requiredString(Map<String, String> routeAttributes, String name) throws UsageException {
        String value = routeAttributes.get(name);
        if ((value == null)) {
            throw new UsageException(String.format("Parameter '%s' is missing", name));
        }
        return value;
    }

    public default int requiredInt(Map<String, String> routeAttributes, String name) throws UsageException {
        String s = requiredString(routeAttributes, name);
        try {
            return Integer.parseInt(s);
        } catch (Exception e) {
            throw new UsageException("Parameter '%s' should be an integer");
        }
    }

    public default long requiredLong(Map<String, String> routeAttributes, String name) throws UsageException {
        String s = requiredString(routeAttributes, name);
        try {
            return Long.parseLong(s);
        } catch (Exception e) {
            throw new UsageException("Parameter '%s' should be an integer");
        }
    }

    public default Optional<String> optionalString(Map<String, String> routeAttributes, String name) {
        String value = routeAttributes.get(name);
        return value == null ? Optional.empty() : Optional.of(value);
    }
}
