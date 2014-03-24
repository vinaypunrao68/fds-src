package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

public interface RequestHandler {
    public Resource handle(Request request) throws Exception;

    public default String requiredString(Request request, String name) throws UsageException {
        if ((request.getParameter(name) == null)) {
            throw new UsageException(String.format("Parameter '%s' is missing", name));
        }
        return request.getParameter(name);
    }

    public default int requiredInt(Request request, String name) throws UsageException {
        String s = requiredString(request, name);
        try {
            return Integer.parseInt(s);
        } catch (Exception e) {
            throw new UsageException("Parameter '%s' should be an integer");
        }
    }

    public default long requiredLong(Request request, String name) throws UsageException {
        String s = requiredString(request, name);
        try {
            return Long.parseLong(s);
        } catch (Exception e) {
            throw new UsageException("Parameter '%s' should be an integer");
        }
    }
}
