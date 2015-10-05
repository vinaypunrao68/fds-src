/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.protocol.FDSP_Uuid;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.pm.NotifyStopServiceMsg;
import com.formationds.protocol.pm.NotifyRemoveServiceMsg;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.io.InputStreamReader;
import java.util.Map;
import java.util.ArrayList;
import java.util.List;

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
        
    	Node node = (new GetNode()).getNode(nodeUuid);
        
        if( node == null ) {
	  		throw new ApiException( "The specified node uuid " + nodeUuid + " has no matching node name.", ErrorCode.MISSING_RESOURCE );
  		}
        
        logger.debug( "Removing {}:{}",
        		node.getName(),
        		nodeUuid );

        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        boolean pmPresent = false;
        
        for(List<Service> svcList : node.getServices().values())
        {
        	for(Service svc : svcList)
        	{
        		SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        				                                 (node.getAddress().getHostAddress(),
                                                          svc);
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
        			                                 (node.getAddress().getHostAddress(),
                                                      pmSvc);
        	svcInfList.add(svcInfo);
        }
        
        // TODO: Fix when we support multiple domains
        
        logger.debug("Stopping and removing services on node");
        int status =
            getConfigApi().StopService(new NotifyStopServiceMsg(svcInfList, true));

        if( status != 0 )
        {
            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.REMOVE_NODE_ERROR,
                                      node.getName(), nodeUuid );
            throw new ApiException( "Error encountered while stopping services on node: "
                    + nodeUuid , ErrorCode.INTERNAL_SERVER_ERROR );
        }
        else 
        {   
            // Now that we have stopped the services go remove them
        	status = getConfigApi().RemoveService(new NotifyRemoveServiceMsg(svcInfList, true));
        	
        	if(status != 0)
        	{
                status= HttpServletResponse.SC_BAD_REQUEST;
                EventManager.notifyEvent( OmEvents.REMOVE_NODE_ERROR,
                                          node.getName(), nodeUuid );
                throw new ApiException( "Error encountered while removing services on node: "
                        + nodeUuid , ErrorCode.INTERNAL_SERVER_ERROR );
        	}
        	else
        	{
                EventManager.notifyEvent( OmEvents.REMOVE_NODE,
                                          node.getName(), nodeUuid );
        	}

        }
        
        return new JsonResource( new JSONObject().put("status", "ok"), HttpServletResponse.SC_OK );
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
