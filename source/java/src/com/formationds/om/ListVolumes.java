package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.apis.*;
import com.formationds.util.JsonArrayCollector;
import com.formationds.util.Size;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Map;

public class ListVolumes implements RequestHandler {
    private ConfigurationService.Iface configApi;
    private AmService.Iface amApi;
    private FDSP_ConfigPathReq.Iface legacyConfig;

    public ListVolumes(ConfigurationService.Iface configApi, AmService.Iface amApi, FDSP_ConfigPathReq.Iface legacyConfig) {
        this.configApi = configApi;
        this.amApi = amApi;
        this.legacyConfig = legacyConfig;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray jsonArray = configApi
                .listVolumes("")
                .stream()
                .map(v -> toJsonObject(v))
                .collect(new JsonArrayCollector());

        return new JsonResource(jsonArray);
    }

    private JSONObject toJsonObject(VolumeDescriptor v) {
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        FDSP_VolumeDescType volInfo = null;
        VolumeStatus status = null;
        try {
            volInfo = legacyConfig.GetVolInfo(msg, new FDSP_GetVolInfoReqType(v.getName(), 0));
            status = amApi.volumeStatus("", v.getName());
        } catch (TException e) {
            throw new RuntimeException(e);
        }

        JSONObject o = new JSONObject();
        o.put("name", v.getName());
        o.put("id", volInfo.getVolUUID());
        o.put("priority", volInfo.getRel_prio());
        o.put("sla", volInfo.getIops_min());
        o.put("limit", volInfo.getIops_max());

        if (v.getPolicy().getVolumeType().equals(VolumeType.OBJECT)) {
            o.put("data_connector", new JSONObject().put("type", "object"));
            o.put("apis", "S3, Swift");
        } else {
            JSONObject connector = new JSONObject().put("type", "block");
            Size size = Size.size(v.getPolicy().getBlockDeviceSizeInBytes());
            JSONObject attributes = new JSONObject()
                    .put("size", size.getCount())
                    .put("unit", size.getSizeUnit().toString());
            connector.put("attributes", attributes);
            o.put("data_connector", connector);
        }

        Size usage = Size.size(status.getCurrentUsageInBytes());
        JSONObject dataUsage = new JSONObject()
                .put("size", usage.getCount())
                .put("unit", usage.getSizeUnit().toString());
        o.put("current_usage", dataUsage);
        return o;
    }
}
