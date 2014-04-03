package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import FDS_ProtocolInterface.*;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;
import java.util.UUID;

public class CreateVolume implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;

    public CreateVolume(FDSP_ConfigPathReq.Iface client) {
        this.client = client;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String name = requiredString(routeParameters, "name");
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        String policyName = name + "_policy";
        int policyId = (int) UUID.randomUUID().getLeastSignificantBits();
        client.CreatePolicy(msg, new FDSP_CreatePolicyType(policyName, new FDSP_PolicyInfoType(policyName,
                policyId,
                0, Double.MAX_VALUE, 0)));
        FDSP_VolumeInfoType vol_info = new FDSP_VolumeInfoType();
        vol_info.setVol_name(name);
        vol_info.setVolPolicyId(policyId);
        int returnValue = client.CreateVol(msg, new FDSP_CreateVolType(name, vol_info));
        return new JsonResource(new JSONObject().put("status", returnValue));
    }
}
