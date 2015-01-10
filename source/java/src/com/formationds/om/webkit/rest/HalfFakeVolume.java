package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_VolumeDescType;

public class HalfFakeVolume {
    private FDSP_VolumeDescType volInfo;

    public HalfFakeVolume(FDSP_VolumeDescType volInfo) {
        super();
        this.volInfo = volInfo;
    }


    public String toString() {
        return             "  {\n" +
                "  \"id\":\"" + volInfo.getVolUUID() + "\",\n" +
                "  \"tenant\":\"" + volInfo.getTennantId() + "\",\n" +
                "  \"name\":\"" + volInfo.getVol_name() + "\",\n" +
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
                "  \"priority\": " + volInfo.getRel_prio() + ",\n" +
                "  \"performance\": 70,\n" +
                "  \"sla\": " + volInfo.getIops_min() + ",\n" +
                "  \"limit\": " + volInfo.getIops_max() + ",\n" +
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
