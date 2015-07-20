/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.Map;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.io.Reader;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.Service.ServiceStatus;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.protocol.pm.NotifyAddServiceMsg;
import com.formationds.protocol.pm.NotifyStartServiceMsg;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.ApiException;
import com.formationds.apis.ConfigurationService;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class AddService implements RequestHandler {

	private static final String NODE_ARG = "node_id";
	
    private static final Logger logger =
            LoggerFactory.getLogger( AddService.class );

    private ConfigurationApi configApi;
	
	public AddService(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
        long nodeId = requiredLong( routeParameters, NODE_ARG );
        logger.debug( "Adding service to node: " + nodeId );
        
        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
        
        Service service = ObjectModelHelper.toObject( reader, Service.class );
        if( service == null ) {
	  		throw new ApiException( "No valid service object provided to add to node. " + nodeId, ErrorCode.MISSING_RESOURCE );
  		}
        
        Node node = (new GetNode()).getNode(nodeId);
          	
        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        
        SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        				               (node.getAddress().getHostAddress(), service);
        svcInfList.add(svcInfo);
        		
        Service pmSvc = (new GetService()).getService(nodeId, nodeId);
        SvcInfo pmSvcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        		                         (node.getAddress().getHostAddress(), pmSvc);
        svcInfList.add(pmSvcInfo);
        
        // Add the new service
        int status =
        		getConfigApi().AddService(new NotifyAddServiceMsg(svcInfList));
                
        if( status != 0 )
        {
            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.ADD_SERVICE_ERROR, 0 );
        }
        else
        {
        	// EventManager.notifyEvent( OmEvents.ADD_SERVICE, 0 );
        }  
        
        long serviceId = retrieveSvcId(service.getType(), nodeId);
        if(serviceId == -1) {
        	throw new ApiException( "Valid service id could not be retrieved for type:" + service.getType(), ErrorCode.MISSING_RESOURCE );
        }
        
        Service newService = (new GetService()).getService(nodeId, serviceId);
        String jsonString = ObjectModelHelper.toJSON( newService );
        return new TextResource( jsonString );
	}

    private ConfigurationApi getConfigApi(){
        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }
    	
        return configApi;
    }
    
    private long retrieveSvcId(ServiceType type, long nodeId){
    	long newSvcId = -1;
        switch( type ){
    	case AM:
    		newSvcId = nodeId + 3;
    		break;
    	case DM:
    		newSvcId = nodeId + 2;
    		break;
    	case SM: 
    		newSvcId = nodeId + 1;
    		break;
    	default:
    		break;
    }
        return newSvcId;
    }

}
