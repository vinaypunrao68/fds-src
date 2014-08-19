package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.util.SizeUnit;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CreateVolume implements RequestHandler {
    private ConfigurationService.Iface configApi;
    private FDSP_ConfigPathReq.Iface legacyConfigPath;

    public CreateVolume(ConfigurationService.Iface configApi, FDSP_ConfigPathReq.Iface legacyConfigPath) {
        this.configApi = configApi;
        this.legacyConfigPath = legacyConfigPath;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String source = IOUtils.toString(request.getInputStream());
        JSONObject o = new JSONObject(source);
        String name = o.getString("name");
        int priority = o.getInt("priority");
        int sla = o.getInt("sla");
        int limit = o.getInt("limit");
        JSONObject connector = o.getJSONObject("data_connector");
        String type = connector.getString("type");
        if ("block".equals(type.toLowerCase())) {
            JSONObject attributes = connector.getJSONObject("attributes");
            int sizeUnits = attributes.getInt("size");
            long sizeInBytes = SizeUnit.valueOf(attributes.getString("unit")).totalBytes(sizeUnits);
            VolumeSettings volumeSettings = new VolumeSettings(1024 * 4, VolumeType.BLOCK, sizeInBytes);
            configApi.createVolume("", name, volumeSettings, 0);
        } else {
            configApi.createVolume("", name, new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0), 0);
        }

        Thread.sleep(200);
        SetVolumeQosParams.setVolumeQos(legacyConfigPath, name, sla, priority, limit);
        return new JsonResource(new JSONObject().put("status", "ok"));
    }
}

