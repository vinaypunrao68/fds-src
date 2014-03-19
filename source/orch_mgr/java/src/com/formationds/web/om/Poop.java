package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

public class Poop implements RequestHandler {
    @Override
    public Resource handle(Request request) throws Exception {
        return new TextResource("poop");
    }
}
