package com.formationds.om.webkit.rest.platform;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 * 
 * This class is for adding a node into an FDS domain
 */

import FDS_ProtocolInterface.FDSP_ActivateOneNodeType;
import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;

import com.formationds.protocol.FDSP_Uuid;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class AddNode
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( AddNode.class );

    private FDSP_ConfigPathReq.Iface client;

    public AddNode( final FDSP_ConfigPathReq.Iface client ) {

        this.client = client;

    }

    @Override
    public Resource handle( final Request request,
                            final Map<String, String> routeParameters)
        throws Exception {

        long nodeUuid = requiredLong(routeParameters, "node_uuid");
        
        Long domainId = requiredLong(routeParameters, "domain_id");

        String source = IOUtils.toString(request.getInputStream());

        JSONObject o = new JSONObject(source);

        boolean activateSm = !o.isNull( "sm" ) && o.getBoolean( "sm" );
        boolean activateAm = !o.isNull( "am" ) && o.getBoolean( "am" );
        boolean activateDm = !o.isNull( "dm" ) && o.getBoolean( "dm" );

        logger.debug( "Activating {} AM: {} DM: {} SM: {}",
                      nodeUuid, activateAm, activateDm, activateSm );

        int status =
            client.ActivateNode( new FDSP_MsgHdrType(),
                                 new FDSP_ActivateOneNodeType(
                                     domainId.intValue(),
                                     new FDSP_Uuid( nodeUuid ),
                                     activateSm,
                                     activateDm,
                                     activateAm ) );

        int httpCode = HttpServletResponse.SC_OK;
        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.ADD_NODE_ERROR,
                                      nodeUuid );

        } else {

            EventManager.notifyEvent( OmEvents.ADD_NODE,
                                      nodeUuid );

        }

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode);
    }
}
