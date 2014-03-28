package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ActivateOneNodeType;
import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Uuid;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;

public class ActivatePlatform implements RequestHandler {
    private FDSP_ConfigPathReq.Iface client;

    public ActivatePlatform(FDSP_ConfigPathReq.Iface client) {
        this.client = client;
    }

    @Override
    public Resource handle(Request request) throws Exception {
        long nodeUuid = requiredLong(request, "node_uuid");
        int domainId = requiredInt(request, "domain_id");

        int status = client.ActivateNode(new FDSP_MsgHdrType(), new FDSP_ActivateOneNodeType(domainId, new FDSP_Uuid(nodeUuid), true, true, true));
        int httpCode = status == 0 ? HttpServletResponse.SC_OK : HttpServletResponse.SC_BAD_REQUEST;
        return new JsonResource(new JSONObject().put("status", status), httpCode);
    }
}
