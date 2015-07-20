/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;
import java.util.ArrayList;
import java.util.List;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.pm.NotifyStopServiceMsg;
import com.formationds.protocol.pm.NotifyStartServiceMsg;
import com.formationds.protocol.svc.types.SvcInfo;
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
		
        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
        Node node = ObjectModelHelper.toObject( reader, Node.class );
        
        Boolean stopNodeServices = false;
        
        switch( node.getState() ){
        	case DOWN:
        		stopNodeServices = true;
        		break;
        	case UP:
        	default:
        		stopNodeServices = false;
        		break;
        }
        
        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        boolean pmPresent = false;
        
        // Get the real node object so we can access other data related to it
    	Node newNode = (new GetNode()).getNode(nodeUuid);
    	
        for(List<Service> svcList : newNode.getServices().values())
        {
        	for(Service svc : svcList)
        	{
        		SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        				              (newNode.getAddress().getHostAddress(), svc);
        		svcInfList.add(svcInfo);
        		
        		if (svc.getType() == ServiceType.PM) {
        			pmPresent = true;
        		}
        		
        	}
        }
        
        if (!pmPresent)
        {
        	Service pmSvc = (new GetService()).getService(nodeUuid, nodeUuid);
        	SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        			              (newNode.getAddress().getHostAddress(), pmSvc);
        	svcInfList.add(svcInfo);
        }
        
        int status = -1;
        
        if ( stopNodeServices )
        {
        	logger.debug("Request to shutdown node, stopping all services, svcList size:" + svcInfList.size());
        	// Note: This action will *not* change the state of the node to "down"
        	// It will however shutdown any existing am/dm/sm services on the node
        	status = getConfigApi().StopService(new NotifyStopServiceMsg(svcInfList));
        	
        	if( status != 0 )
            {
                status= HttpServletResponse.SC_BAD_REQUEST;
                EventManager.notifyEvent( OmEvents.CHANGE_NODE_STATE_FAILED,
                                          nodeUuid );
            }
            else 
            {
            	//EventManager.notifyEvent( OmEvents.STOP_NODE, nodeUuid );
            }
        }
        else
        {
        	logger.debug("Request to start node, starting all services, svcList size:" + svcInfList.size());
        	status = getConfigApi().StartService(new NotifyStartServiceMsg(svcInfList));
        	
        	if( status != 0 )
            {
                status= HttpServletResponse.SC_BAD_REQUEST;
                EventManager.notifyEvent( OmEvents.CHANGE_NODE_STATE_FAILED,
                                          nodeUuid );
            }
            else 
            {
            	//EventManager.notifyEvent( OmEvents.START_NODE, nodeUuid );
            }
        }
        
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
