package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

public interface RequestHandler {
    public Resource handle(Request request) throws Exception;

    public default String assertParameter(Request request, String name) throws UsageException {
        if ((request.getParameter(name) == null)) {
            throw new UsageException(String.format("Parameter %s is missing", name));
        }
        return request.getParameter(name);
    }
}
