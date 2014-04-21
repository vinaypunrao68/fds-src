package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class FancyCreateVolume implements RequestHandler {
    private FDSP_ConfigPathReq.Iface iface;

    public FancyCreateVolume(FDSP_ConfigPathReq.Iface iface) {
        this.iface = iface;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String s = IOUtils.toString(request.getInputStream());
        JSONObject jsonObject = new JSONObject(s);
        String name = jsonObject.getString("name");
        int minIops = jsonObject.getInt("sla");
        int priority = jsonObject.getInt("priority");
        int maxIops = jsonObject.getInt("limit");

        int response = new CreateVolume(iface).createVolume(name, minIops, maxIops, priority);
        return new JsonResource(new JSONObject().put("status", response));
    }
}
