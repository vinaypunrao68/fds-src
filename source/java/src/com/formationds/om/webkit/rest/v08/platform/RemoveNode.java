/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.protocol.FDSP_Uuid;
import com.formationds.apis.ApiException;
import com.formationds.apis.ErrorCode;
import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

public class RemoveNode
    implements RequestHandler {

	private static final String NODE_ARG = "node_id";
	
    private static final Logger logger =
        LoggerFactory.getLogger( RemoveNode.class );

    private ConfigurationApi configApi;

    public RemoveNode() {}

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters )
        throws Exception {
    	
    	long nodeUuid = requiredLong(routeParameters, NODE_ARG );
        
        final InputStreamReader reader = new InputStreamReader( request.getInputStream() );
        Node node = ObjectModelHelper.toObject( reader, Node.class );
        
        if( node == null ) {
	  		throw new ApiException( "The specified node uuid " + nodeUuid + " has no matching node name.", ErrorCode.MISSING_RESOURCE );
  		}
        
        logger.debug( "Deactivating {}:{}",
        		node.getName(),
        		nodeUuid );

        // TODO: Fix when we support multiple domains
        
        int status =
            getConfigApi().RemoveServices( new FDSP_RemoveServicesType(
                                     node.getName(),
                                     new FDSP_Uuid( nodeUuid ),
                                     true,
                                     true,
                                     true ) );
        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.ADD_NODE_ERROR,
                                      nodeUuid );

        } else {

            EventManager.notifyEvent( OmEvents.ADD_NODE,
                                      nodeUuid );

        }

        List<Node> nodes = (new ListNodes()).getNodes();
        
        String jsonString = ObjectModelHelper.toJSON( nodes );
        
        return new TextResource( jsonString );
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
