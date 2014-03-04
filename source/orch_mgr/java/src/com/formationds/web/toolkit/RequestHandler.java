package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.servlet.http.HttpServletRequest;

public interface RequestHandler {
    public Resource handle(HttpServletRequest request) throws Exception;
}
