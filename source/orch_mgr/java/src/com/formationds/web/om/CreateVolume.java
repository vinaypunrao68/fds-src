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
        String name = requiredString(routeParameters, "volume");
        int returnValue = createVolume(name, 0, 1000, 1);
        return new JsonResource(new JSONObject().put("status", returnValue));
    }

    public int createVolume(String name, int minIops, int maxIops, int priority) throws org.apache.thrift.TException {
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        String policyName = name + "_policy";
        int policyId = (int) UUID.randomUUID().getLeastSignificantBits();
        client.CreatePolicy(msg, new FDSP_CreatePolicyType(policyName, new FDSP_PolicyInfoType(policyName,
                policyId,
                minIops,
                maxIops,
                priority)));
        FDSP_VolumeInfoType volInfo = new FDSP_VolumeInfoType();
        volInfo.setVol_name(name);
        volInfo.setVolPolicyId(policyId);
        return client.CreateVol(msg, new FDSP_CreateVolType(name, volInfo));
    }
}
