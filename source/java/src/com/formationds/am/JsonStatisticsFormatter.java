package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.streaming.DataPointPair;
import com.formationds.streaming.volumeDataPoints;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

public class JsonStatisticsFormatter {

    public JSONArray format(List<volumeDataPoints> dataPoints) {
        JSONArray results = new JSONArray();
        dataPoints.stream()
                .flatMap(vdp -> vdp.getMeta_list().stream().map(dpp -> toJsonObject(vdp.getVolume_name(), vdp.getTimestamp(), dpp)))
                .forEach(o -> results.put(o));

        return results;
    }

    private JSONObject toJsonObject(String volumeName, long timestamp, DataPointPair dpp) {
        return new JSONObject()
                .put("volume", volumeName)
                .put("timestamp", Long.toString(timestamp))
                .put("key", dpp.getKey())
                .put("value", Double.toString(dpp.getValue()));
    }

}
