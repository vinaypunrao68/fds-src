package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class ActivatePlatform implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;

    public ActivatePlatform(FDSP_ConfigPathReq.Iface client) {
        this.client = client;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long nodeUuid = requiredLong(routeParameters, "node_uuid");
        int domainId = requiredInt(routeParameters, "domain_id");

        long dms = client.ListServices(new FDSP_MsgHdrType())
                .stream()
                .filter(n -> n.getNode_type() == FDSP_MgrIdType.FDSP_DATA_MGR)
                .count();

        boolean activateDm = dms == 0;

        int status = client.ActivateNode(new FDSP_MsgHdrType(), new FDSP_ActivateOneNodeType(domainId, new FDSP_Uuid(nodeUuid), true, activateDm, true));
        int httpCode = status == 0 ? HttpServletResponse.SC_OK : HttpServletResponse.SC_BAD_REQUEST;
        return new JsonResource(new JSONObject().put("status", status), httpCode);
    }
}
