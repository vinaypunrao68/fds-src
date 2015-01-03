package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
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
        String source = IOUtils.toString(request.getInputStream());
        JSONObject o = new JSONObject(source);
        boolean activateSm = o.getBoolean("sm");
        boolean activateAm = o.getBoolean("am");

        long dmCount = client.ListServices(new FDSP_MsgHdrType())
                .stream()
                .filter(n -> n.getNode_type() == FDSP_MgrIdType.FDSP_DATA_MGR)
                .count();

        // We only support one dm now
        boolean activateDm = o.getBoolean("dm") && dmCount == 0;

            int status = client.ActivateNode(new FDSP_MsgHdrType(), new FDSP_ActivateOneNodeType(1, new FDSP_Uuid(nodeUuid), activateSm, activateDm, activateAm));
        int httpCode = status == 0 ? HttpServletResponse.SC_OK : HttpServletResponse.SC_BAD_REQUEST;
        return new JsonResource(new JSONObject().put("status", status), httpCode);
    }
}
