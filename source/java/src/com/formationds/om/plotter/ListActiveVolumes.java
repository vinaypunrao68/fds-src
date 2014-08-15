package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.Map;

public class ListActiveVolumes implements RequestHandler {
    private VolumeStatistics volumeStatistics;

    public ListActiveVolumes(VolumeStatistics volumeStatistics) {
        this.volumeStatistics = volumeStatistics;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray();
        volumeStatistics.volumes().forEach(v -> array.put(v));
        return new JsonResource(array);
    }
}
