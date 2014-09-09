package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.apis.AmService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.VolumeType;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.JsonArrayCollector;
import com.formationds.util.Size;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.text.DecimalFormat;
import java.util.Map;

public class ListVolumes implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(ListVolumes.class);

    private Xdi xdi;
    private AmService.Iface amApi;
    private FDSP_ConfigPathReq.Iface legacyConfig;
    private AuthenticationToken token;

    private static DecimalFormat df = new DecimalFormat("#.00");

    public ListVolumes(Xdi xdi, AmService.Iface amApi, FDSP_ConfigPathReq.Iface legacyConfig, AuthenticationToken token) {
        this.xdi = xdi;
        this.amApi = amApi;
        this.legacyConfig = legacyConfig;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray jsonArray = xdi.listVolumes(token, "")
                .stream()
                .map(v -> {
                    try {
                        return toJsonObject(v);
                    } catch (TException e) {
                        LOG.error("Error fetching configuration data for volume", e);
                        throw new RuntimeException(e);
                    }
                })
                .collect(new JsonArrayCollector());

        return new JsonResource(jsonArray);
    }

    private JSONObject toJsonObject(VolumeDescriptor v) throws TException {
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        FDSP_VolumeDescType volInfo = null;
        VolumeStatus status = null;
        volInfo = legacyConfig.GetVolInfo(msg, new FDSP_GetVolInfoReqType(v.getName(), 0));
        status = amApi.volumeStatus("", v.getName());

        return toJsonObject(v, volInfo, status);
    }

    public static JSONObject toJsonObject(VolumeDescriptor v, FDSP_VolumeDescType volInfo, VolumeStatus status) {
        JSONObject o = new JSONObject();
        o.put("name", v.getName());
        o.put("id", Long.toString(volInfo.getVolUUID()));
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
                .put("size", formatSize(usage))
                .put("unit", usage.getSizeUnit().toString());
        o.put("current_usage", dataUsage);
        return o;
    }

    private static String formatSize(Size usage) {
        if (usage.getCount() == 0) {
            return "0";
        }

        return df.format(usage.getCount());
    }
}
