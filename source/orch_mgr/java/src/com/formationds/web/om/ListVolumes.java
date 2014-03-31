package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.base.Joiner;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;

import java.util.List;

public class ListVolumes implements RequestHandler {
    private FDSP_ConfigPathReq.Iface iface;

    public ListVolumes(FDSP_ConfigPathReq.Iface iface) {
        this.iface = iface;
    }

    @Override
    public Resource handle(Request request) throws Exception {
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        List<FDSP_VolumeDescType> volumes = iface.ListVolumes(msg);
        String[] descriptions = volumes.stream()
                .map(v -> {
                    try {
                        FDSP_VolumeDescType volInfo = iface.GetVolInfo(msg, new FDSP_GetVolInfoReqType(v.getVol_name(), v.getGlobDomainId()));
                        return makeHalfFakeVolume(v, volInfo);

                    } catch (TException e) {
                        throw new RuntimeException(e);
                    }
                })
                .toArray(i -> new String[i]);
        return new TextResource("[" + Joiner.on(",\n").join(descriptions) + "]");
    }

    private String makeHalfFakeVolume(FDSP_VolumeDescType volume, FDSP_VolumeDescType volInfo) {
        return             "  {\n" +
                "  \"id\":\"" + volume.getVolUUID() + "\",\n" +
                "  \"tenant\":\"" + volume.getTennantId() + "\",\n" +
                "  \"name\":\"" + volume.getVol_name() + "\",\n" +
                "  \"consumption\": 1.2,\n" +
                "  \"usage_history\":\n" +
                "    [\n" +
                "      [\"2012-11-15\", 2 ],\n" +
                "      [\"2012-11-16\", 3 ],\n" +
                "      [\"2012-11-17\", 5 ],\n" +
                "      [\"2012-11-18\", 6 ],\n" +
                "      [\"2012-11-19\", 7 ],\n" +
                "      [\"2012-11-20\", 10 ],\n" +
                "      [\"2012-11-21\", 11 ],\n" +
                "      [\"2012-11-22\", 13 ],\n" +
                "      [\"2012-11-23\", 12 ],\n" +
                "      [\"2012-11-24\", 12 ],\n" +
                "      [\"2012-11-25\", 14 ],\n" +
                "      [\"2012-11-26\", 15 ],\n" +
                "      [\"2012-11-27\", 15 ],\n" +
                "      [\"2012-11-28\", 16 ],\n" +
                "      [\"2012-11-29\", 18 ],\n" +
                "      [\"2012-11-30\", 19 ]\n" +
                "    ],\n" +
                "  \"capacity\": [\n" +
                "    [\"2012-11-15\", 2 ],\n" +
                "    [\"2012-11-16\", 3 ],\n" +
                "    [\"2012-11-17\", 5 ],\n" +
                "    [\"2012-11-18\", 5 ],\n" +
                "    [\"2012-11-19\", 4 ],\n" +
                "    [\"2012-11-20\", 6 ],\n" +
                "    [\"2012-11-21\", 5 ],\n" +
                "    [\"2012-11-22\", 7 ],\n" +
                "    [\"2012-11-23\", 5 ],\n" +
                "    [\"2012-11-24\", 4 ],\n" +
                "    [\"2012-11-25\", 4 ],\n" +
                "    [\"2012-11-26\", 5 ],\n" +
                "    [\"2012-11-27\", 5 ],\n" +
                "    [\"2012-11-28\", 6 ],\n" +
                "    [\"2012-11-29\", 6 ],\n" +
                "    [\"2012-11-30\", 5 ]\n" +
                "  ],\n" +
                "  \"usage\": \"3Tb\",\n" +
                "  \"status\": [\"\"],\n" +
                "  \"priority\": 1,\n" +
                "  \"performance\": 70,\n" +
                "  \"sla\": 40,\n" +
                "  \"limit\": 100,\n" +
                "  \"limiting\": 0,\n" +
                "  \"data_connector\": {\n" +
                "    \"type\": \"block\",\n" +
                "    \"block_data_capacity\": \"3Tb\"\n" +
                "    },\n" +
                "  \"domains\": {\n" +
                "    \"primary\": {\n" +
                "      \"domain\": \"FS.001\",\n" +
                "      \"access_point\": \"Lawndale\",\n" +
                "      \"protection\": \"3-way Redundant\",\n" +
                "      \"resiliency\": {\n" +
                "        \"continuous\": \"Dynamic\",\n" +
                "        \"daily\": \"5 days\",\n" +
                "        \"weekly\": \"2 weeks\",\n" +
                "        \"monthly\": \"3 months\",\n" +
                "        \"retention\": \"7 years\"\n" +
                "        }\n" +
                "      }\n" +
                "    },\n" +
                "  \"data_lifecycle\": \"Auto\"\n" +
                "  }\n";

    }
}
