package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class SetVolumeQosParams implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;

    public SetVolumeQosParams(FDSP_ConfigPathReq.Iface client) {
        this.client = client;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long uuid = Long.parseLong(requiredString(routeParameters, "uuid"));
        JSONObject jsonObject = new JSONObject(IOUtils.toString(request.getInputStream()));
        int minIops = jsonObject.getInt("sla");
        int priority = jsonObject.getInt("priority");
        int maxIops = jsonObject.getInt("limit");

        FDSP_VolumeDescType volumeDescType = client.ListVolumes(new FDSP_MsgHdrType())
                .stream()
                .filter(v -> v.getVolUUID() == uuid)
                .findFirst()
                .orElseThrow(() -> new RuntimeException("No such volume"));

        FDSP_VolumeDescType volInfo = client.GetVolInfo(new FDSP_MsgHdrType(), new FDSP_GetVolInfoReqType(volumeDescType.getVol_name(), 0));
        volInfo.setIops_min(minIops);
        volInfo.setRel_prio(priority);
        volInfo.setIops_max(maxIops);
        volInfo.setVolPolicyId(0);
        client.ModifyVol(new FDSP_MsgHdrType(), new FDSP_ModifyVolType(volInfo.getVol_name(),
                volInfo.getVolUUID(),
                volInfo));

        return new TextResource(new HalfFakeVolume(volInfo).toString()) {
            @Override
            public String getContentType() {
                return "application/json";
            }
        };
    }
}
