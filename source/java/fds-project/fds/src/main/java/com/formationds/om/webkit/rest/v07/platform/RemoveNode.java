/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.platform;

import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.commons.model.Node;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.List;
import java.util.Map;

public class RemoveNode
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( RemoveNode.class );

    private ConfigurationService.Iface client;

    public RemoveNode( ConfigurationService.Iface client ) {

        this.client = client;

    }

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters )
        throws Exception {

        final Long nodeUuid = requiredLong( routeParameters, "node_uuid" );
        
        List<com.formationds.protocol.svc.types.FDSP_Node_Info_Type> list = client.ListServices( 0 );
        Map<String, Node> nodeMap = (new ListNodes( client )).computeNodeMap(list);

        Node node = nodeMap.get( nodeUuid.toString() );
        
        if( node == null ) {

            throw new Exception( "The specified node uuid " + nodeUuid +
                                 " has no matching node name." );

        }

        logger.debug( "Deactivating {}:{}",
                      node.getName(),
                      nodeUuid);

        //TODO: Have a method to actually remove a node instead of just messing with services
        // since we are removing the node, for now we're removing all the services.
        int status = 
            client.RemoveServices( new FDSP_RemoveServicesType(
                                     node.getName(),
                                     new FDSP_Uuid( nodeUuid ),
                                     true,
                                     true,
                                     true ) );

        int httpCode = HttpServletResponse.SC_OK;
        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.REMOVE_NODE_ERROR,
                                      node.getName(),
                                      nodeUuid );
            
            logger.error("Node removal failed.");

        } else {

            EventManager.notifyEvent( OmEvents.REMOVE_NODE,
                                      node.getName(),
                                      nodeUuid );
            logger.info( "Node successfully removed from the system.");

        }

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode );
    }
}
