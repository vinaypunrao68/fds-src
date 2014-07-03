package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_DeleteVolType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeleteVolume implements RequestHandler {
    private FDSP_ConfigPathReq.Iface iface;

    public DeleteVolume(FDSP_ConfigPathReq.Iface iface) {
        this.iface = iface;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String volumeName = requiredString(routeParameters, "name");
        int result = iface.DeleteVol(new FDSP_MsgHdrType(), new FDSP_DeleteVolType(volumeName, 0));
        return new JsonResource(new JSONObject().put("status", result));
    }
}
