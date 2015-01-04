package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Map;

/**
 * @deprecated the POC functionality has been replaced by
 *             {@link com.formationds.om.webkit.rest.metrics.IngestVolumeStats}
 *
 *             will be removed in the future
 */
@Deprecated
public class RegisterVolumeStats implements RequestHandler {
    private VolumeStatistics volumeStatistics;

    public RegisterVolumeStats(VolumeStatistics volumeStatistics) {
        this.volumeStatistics = volumeStatistics;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray(IOUtils.toString(request.getInputStream()));
        for (int i = 0; i < array.length(); i++) {
            JSONObject jsonObject = array.getJSONObject(i);
            volumeStatistics.put(new VolumeDatapoint(jsonObject));
        }
        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
