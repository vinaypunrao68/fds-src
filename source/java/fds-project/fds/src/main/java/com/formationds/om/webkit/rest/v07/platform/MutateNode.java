package com.formationds.om.webkit.rest.v07.platform;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.FDSP_ActivateOneNodeType;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class MutateNode implements RequestHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( AddNode.class );

    private ConfigurationService.Iface client;
	
	public MutateNode( final ConfigurationService.Iface client ){
		
		this.client = client;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
        Long nodeUuid = requiredLong(routeParameters, "node_uuid");
		
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
        
        //TODO:  Have a call that changes the node's state!
        int status = 
        		client.ActivateNode( new FDSP_ActivateOneNodeType(
                                          1,
                                          new FDSP_Uuid( nodeUuid ),
                                          activateSm,
                                          activateDm,
                                          activateAm ) );

        int httpCode = HttpServletResponse.SC_OK;
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

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode);
	}

}
