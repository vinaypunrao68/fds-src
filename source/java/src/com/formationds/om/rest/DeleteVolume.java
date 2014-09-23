package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeleteVolume implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteVolume(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String volumeName = requiredString(routeParameters, "name");

        // TODO shouldn't we change to the "real" user token, instead of anonymous
        xdi.deleteVolume(AuthenticationToken.ANONYMOUS, "", volumeName);
        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
