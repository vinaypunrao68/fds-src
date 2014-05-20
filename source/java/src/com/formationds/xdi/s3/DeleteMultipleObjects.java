package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class DeleteMultipleObjects implements RequestHandler {
    public DeleteMultipleObjects(Xdi xdi) {
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return null;
    }
}
