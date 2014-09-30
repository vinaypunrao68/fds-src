package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;
import java.util.Map;

public class ListStreams implements RequestHandler {
    private ConfigurationService.Iface configApi;

    public ListStreams(ConfigurationService.Iface configApi) {
        this.configApi = configApi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        List<StreamingRegistrationMsg> regs = configApi.getStreamRegistrations(0);
        JSONArray result = new JSONArray();
        for (StreamingRegistrationMsg reg : regs) {
            result.put(asObject(reg));
        }
        return new JsonResource(result);
    }

    private JSONObject asObject(StreamingRegistrationMsg msg) {
        JSONObject o = new JSONObject();
        o.put("id", msg.getId());
        o.put("url", msg.getUrl());
        o.put("method", msg.getHttp_method());

        JSONArray volumeNames = new JSONArray();

        for (String volumeName : msg.getVolume_names()) {
            volumeNames.put(volumeName);
        }
        o.put("volumes", volumeNames);
        o.put("frequency", msg.getSample_freq_seconds());
        o.put("duration", msg.getDuration_seconds());

        return o;
    }
}
