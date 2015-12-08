/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.Map;
import java.util.List;
import java.util.ArrayList;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.model.Service.ServiceState;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.formationds.protocol.pm.NotifyRemoveServiceMsg;
import com.formationds.protocol.pm.NotifyStopServiceMsg;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class RemoveService implements RequestHandler {

	private static final String NODE_ID = "node_id";
	private static final String SERVICE_ID = "service_id";
	
    private static final Logger logger =
            LoggerFactory.getLogger( RemoveService.class );

    private ConfigurationApi configApi;
	
	public RemoveService(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
	
		long nodeId = requiredLong( routeParameters, NODE_ID );
		long serviceId = requiredLong( routeParameters, SERVICE_ID );
		
		logger.debug( "Removing service {} from node {}.", serviceId, nodeId );
		
		Node node = (new GetNode()).getNode( nodeId );
		
		// TODO: Yes, this is redundant but it's easy.  We should fix the thrift part of this insanity
		// instead of focusing on this.
		Service service = (new GetService()).getService( nodeId, serviceId );
	
        if ( service.getStatus().getServiceState() != ServiceState.NOT_RUNNING )
        {
        	throw new ApiException("Trying to remove a service that has not been stopped!",
        			               ErrorCode.BAD_REQUEST);
        }
        
        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        
        SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        				               (node.getAddress().getHostAddress(), service);
        svcInfList.add(svcInfo);
        		
        Service pmSvc = (new GetService()).getService(nodeId, nodeId);
        SvcInfo pmSvcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        		                         (node.getAddress().getHostAddress(), pmSvc);
        svcInfList.add(pmSvcInfo);

		if( ( new RemoveNode() ).hasNonPMServices( svcInfList ) )
		{
			int status = getConfigApi( ).RemoveService(
					new NotifyRemoveServiceMsg( svcInfList, false ) );

			if ( status != 0 )
			{

				status = HttpServletResponse.SC_BAD_REQUEST;
				EventManager.notifyEvent( OmEvents.REMOVE_SERVICE_ERROR, serviceId );

				throw new ApiException( "Remove service failed.", ErrorCode.INTERNAL_SERVER_ERROR );
			} else
			{
				EventManager.notifyEvent( OmEvents.REMOVE_SERVICE, serviceId );
			}

			return new JsonResource( new JSONObject( ).put( "status", "ok" ),
									 HttpServletResponse.SC_OK );
		}

		final String message = "The specified service[ " + serviceId + " ] is not active on PM uuid[ " + nodeId + " ].";
		logger.debug( message );
		throw new ApiException( message, ErrorCode.MISSING_RESOURCE );
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
