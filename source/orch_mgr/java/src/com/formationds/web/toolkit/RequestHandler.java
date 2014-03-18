package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletRequest;

public interface RequestHandler {
    public Resource handle(Request request) throws Exception;
}
