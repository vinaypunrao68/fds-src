package com.formationds.web.om;

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import javax.servlet.http.HttpServletRequest;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class LandingPage extends TextResource implements RequestHandler {
    public LandingPage() {
        super("it worked!");
    }

    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        return this;
    }
}
