package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class SetVolumeQosParams implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;

    public SetVolumeQosParams(FDSP_ConfigPathReq.Iface client) {
        this.client = client;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        // /api/config/volumes/:volume/qos/:min/:priority/:max
        String volumeName = requiredString(routeParameters, "volume");
        FDSP_VolumeDescType volInfo = client.GetVolInfo(new FDSP_MsgHdrType(), new FDSP_GetVolInfoReqType(volumeName, 0));
        volInfo.setIops_min(requiredInt(routeParameters, "min"));
        volInfo.setRel_prio(requiredInt(routeParameters, "priority"));
        volInfo.setIops_max(requiredInt(routeParameters, "max"));
        client.ModifyVol(new FDSP_MsgHdrType(), new FDSP_ModifyVolType(volumeName, volInfo.getVolUUID(), new FDSP_VolumeDescType()));
        return new TextResource(new HalfFakeVolume(volInfo).toString()) {
            @Override
            public String getContentType() {
                return "application/json";
            }
        };
    }
}
