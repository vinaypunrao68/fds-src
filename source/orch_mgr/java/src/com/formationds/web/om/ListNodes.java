package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Node_Info_Type;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.codehaus.jackson.map.ObjectMapper;

import javax.servlet.http.HttpServletRequest;
import java.util.List;

public class ListNodes implements RequestHandler {
    private FDSP_ConfigPathReq.Iface configPathClient;

    public ListNodes(FDSP_ConfigPathReq.Iface configPathClient) {
        this.configPathClient = configPathClient;
    }

    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        List<FDSP_Node_Info_Type> list = configPathClient.ListServices(new FDSP_MsgHdrType());
        return new TextResource(new ObjectMapper().writeValueAsString(list));
    }
}
