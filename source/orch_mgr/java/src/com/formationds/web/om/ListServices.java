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
import com.formationds.web.toolkit.TextResource;
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
        return new TextResource(canned) {
            @Override
            public String getContentType() {
                return "application/json";
            }
        };
    }

    public Resource _handle(Request request) throws Exception {
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

    String canned = " [\n" +
            "  {\n" +
            "  node_name: \"10.1.10.106\",\n" +
            "  site: \"Fremont\",\n" +
            "  node_type: \"FDSP_STOR_MGR\",\n" +
            "  service_uuid: 6868966136943370,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943370,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.106\",\n" +
            "  site: \"Fremont\",\n" +
            "  node_type: \"FDSP_STOR_HVISOR\",\n" +
            "  service_uuid: 6868966136943370,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943370,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.106\",\n" +
            "  site: \"Fremont\",\n" +
            "  node_type: \"FDSP_DATA_MGR\",\n" +
            "  service_uuid: 6868966136943370,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943370,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.106\",\n" +
            "  site: \"Fremont\",\n" +
            "  node_type: \"FDSP_PLATFORM\",\n" +
            "  service_uuid: 6868966136943370,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943370,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.115\",\n" +
            "  site: \"Boulder\",\n" +
            "  node_type: \"FDSP_PLATFORM\",\n" +
            "  service_uuid: 6868966136943371,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943371,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.126\",\n" +
            "  site: \"SF\",\n" +
            "  node_type: \"FDSP_PLATFORM\",\n" +
            "  service_uuid: 6868966136943372,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943372,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.116\",\n" +
            "  site: \"New York\",\n" +
            "  node_type: \"FDSP_PLATFORM\",\n" +
            "  service_uuid: 6868966136943373,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943373,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.129\",\n" +
            "  site: \"Seattle\",\n" +
            "  node_type: \"FDSP_STOR_MGR\",\n" +
            "  service_uuid: 6868966136943379,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Up\",\n" +
            "  node_uuid: 6868966136943379,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.129\",\n" +
            "  site: \"Seattle\",\n" +
            "  node_type: \"FDSP_STOR_HVISOR\",\n" +
            "  service_uuid: 6868966136943379,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Down\",\n" +
            "  node_uuid: 6868966136943379,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.129\",\n" +
            "  site: \"Seattle\",\n" +
            "  node_type: \"FDSP_DATA_MGR\",\n" +
            "  service_uuid: 6868966136943379,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Inactive\",\n" +
            "  node_uuid: 6868966136943379,\n" +
            "  node_id: 0\n" +
            "  },\n" +
            "  {\n" +
            "  node_name: \"10.1.10.129\",\n" +
            "  site: \"Seattle\",\n" +
            "  node_type: \"FDSP_PLATFORM\",\n" +
            "  service_uuid: 6868966136943379,\n" +
            "  domain: \"Formation Data Systems\",\n" +
            "  node_state: \"FDS_Node_Down\",\n" +
            "  node_uuid: 6868966136943379,\n" +
            "  node_id: 0\n" +
            "  }\n" +
            "]";
}
