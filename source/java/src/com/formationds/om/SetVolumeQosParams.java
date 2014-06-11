package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class SetVolumeQosParams implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;
    private ConfigurationService.Iface configService;
    private AmService.Iface amService;

    public SetVolumeQosParams(FDSP_ConfigPathReq.Iface legacyClient, ConfigurationService.Iface configService, AmService.Iface amService) {
        this.client = legacyClient;
        this.configService = configService;
        this.amService = amService;
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

        String volumeName = volumeDescType.getVol_name();
        FDSP_VolumeDescType volInfo = setVolumeQos(client, volumeName, minIops, priority, maxIops);
        VolumeDescriptor descriptor = configService.statVolume("", volumeName);
        JSONObject o = ListVolumes.toJsonObject(descriptor, volInfo, amService.volumeStatus("", volumeName));
        return new JsonResource(o);
    }

    public static  FDSP_VolumeDescType setVolumeQos(FDSP_ConfigPathReq.Iface client, String volumeName, int minIops, int priority, int maxIops) throws org.apache.thrift.TException {
        FDSP_VolumeDescType volInfo = client.GetVolInfo(new FDSP_MsgHdrType(), new FDSP_GetVolInfoReqType(volumeName, 0));
        volInfo.setIops_min(minIops);
        volInfo.setRel_prio(priority);
        volInfo.setIops_max(maxIops);
        volInfo.setVolPolicyId(0);
        client.ModifyVol(new FDSP_MsgHdrType(), new FDSP_ModifyVolType(volInfo.getVol_name(),
                volInfo.getVolUUID(),
                volInfo));
        return volInfo;
    }
}
