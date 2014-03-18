package com.formationds.web.om;

import com.formationds.web.toolkit.HttpRedirect;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import javax.servlet.http.HttpServletRequest;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class LandingPage implements RequestHandler {
    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        return new HttpRedirect("index.html");
    }
}
