package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Node_Info_Type;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsonorg.JsonOrgModule;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.List;

public class ListServices implements RequestHandler {
    private FDSP_ConfigPathReq.Iface configPathClient;

    public ListServices(FDSP_ConfigPathReq.Iface configPathClient) {
        this.configPathClient = configPathClient;
    }

    @Override
    public Resource handle(Request request) throws Exception {
        List<FDSP_Node_Info_Type> list = configPathClient.ListServices(new FDSP_MsgHdrType());
        ObjectMapper mapper = new ObjectMapper();
        mapper.registerModule(new JsonOrgModule());
        JSONArray jsonArray = mapper.convertValue(list, JSONArray.class);
        for (int i = 0; i < jsonArray.length(); i++) {
            jsonArray.getJSONObject(i)
                    .put("site", "Fremont")
                    .put("domain", "Formation Data Systems");
        }
        return new JsonResource(jsonArray);
    }
}
