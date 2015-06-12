/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.FDSP_ActivateOneNodeType;
import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.FDSP_Uuid;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class MutateNode implements RequestHandler {

	private static final String NODE_ARG = "node_id";
	
    private static final Logger logger =
            LoggerFactory.getLogger( AddNode.class );

    private ConfigurationApi configApi;
	
	public MutateNode(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
        Long nodeUuid = requiredLong(routeParameters, NODE_ARG );
        
        logger.debug( "Changing something about node: " + nodeUuid );
		
        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
        Node node = ObjectModelHelper.toObject( reader, Node.class );
      
        Boolean activateAm = true;
        Boolean activateSm = true;
        Boolean activateDm = true;
        OmEvents event = null;
        
        switch( node.getState() ){
        	case DOWN:
        		activateAm = false;
        		activateSm = false;
        		activateDm = false;
        		event = OmEvents.STOP_NODE;
        		break;
        	case UP:
        	default:
        		event = OmEvents.START_NODE;
        		break;
        }
        
        logger.debug( "Attempting state change - AM: " + activateAm + " SM: " + activateSm + " DM: " + activateDm );
        
        //TODO:  Have a call that changes the node's state!
        int status = 
        		getConfigApi().ActivateNode( new FDSP_ActivateOneNodeType(
                                          1,
                                          new FDSP_Uuid( nodeUuid ),
                                          activateSm,
                                          activateDm,
                                          activateAm ) );

        if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.CHANGE_NODE_STATE_FAILED,
                                      nodeUuid );
            logger.error( "Node failed to change state." );

        } else {

        	
            EventManager.notifyEvent(event,
                                      nodeUuid );
            logger.info( "Node state changed successfully.");

        }

        Node newNode = (new GetNode()).getNode( nodeUuid );
        
        String jsonString = ObjectModelHelper.toJSON( newNode );
        
        return new TextResource( jsonString );
	}

	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
	
}
