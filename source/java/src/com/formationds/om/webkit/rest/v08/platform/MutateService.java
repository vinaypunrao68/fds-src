/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.apis.FDSP_ActivateOneNodeType;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.Service.ServiceStatus;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.FDSP_Uuid;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class MutateService implements RequestHandler {

	private static final String NODE_ARG = "node_id";
	private static final String SERVICE_ARG = "service_id";
	
    private static final Logger logger = LoggerFactory.getLogger( MutateService.class );

    private ConfigurationApi configApi;
	
	public MutateService(){}
	
	/**
	 * TODO: FIX!
	 * 
	 * This is an amazing hack. Basically we should be able to take a service, find it
	 * by id and issue the correct request.  However, the system doesn't allow
	 * this so we need to find the node, find the current status of other services,
	 * identify the one we're changing and ship the whole ball of was back.
	 * 
	 * Awesome.
	 */
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
        Long nodeId = requiredLong(routeParameters, NODE_ARG );        
        Long serviceId = requiredLong(routeParameters, SERVICE_ARG);

        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
        
        Service service = ObjectModelHelper.toObject( reader, Service.class );
        service.setId( serviceId );
        
        Node node = (new GetNode()).getNode( nodeId );
        
        Boolean am = isServiceOnAlready( ServiceType.AM, node );
        Boolean sm = isServiceOnAlready( ServiceType.SM, node );
        Boolean dm = isServiceOnAlready( ServiceType.DM, node );
        
        Boolean myState = convertState( service.getStatus() );
        
        switch( service.getType() ){
        	case AM:
        		am = myState;
        		break;
        	case DM:
        		dm = myState;
        		break;
        	case SM: 
        		sm = myState;
        		break;
        	default:
        		break;
        }
        
        int status = getConfigApi().ActivateNode( new FDSP_ActivateOneNodeType(
        		0, 
        		new FDSP_Uuid( node.getId() ), 
        		sm, 
        		dm, 
        		am ));

        if ( status != 0 ){
        	
            status = HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE_ERROR,
                                      nodeId );
            logger.error( "Service state changed failed." );
            
            throw new ApiException( "Service state change failed.", ErrorCode.INTERNAL_SERVER_ERROR );
        }
        else {
            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE,
                    nodeId );
            logger.info( "Service state changed successfully." );
        }
        
        List<Service> services = (new ListServices()).getServicesForNode( nodeId );
        
        String jsonString = ObjectModelHelper.toJSON( services );
	       
        return new TextResource( jsonString );
	}
	
	
	/**
	 * Find the current condition of the AM, SM and DM service
	 * 
	 * @param ams
	 * @return
	 */
	private Boolean isServiceOnAlready( ServiceType type, Node node ){
		
		List<Service> services = node.getServices().get( type );
		
		if ( services == null || services.size() == 0 ){
			return false;
		}
		
		Service service = services.get( 0 );
		
		return convertState( service.getStatus() );
	}
	
	private Boolean convertState( ServiceStatus status ){
		
		switch( status ){
			case RUNNING:
			case DEGRADED:
			case LIMITED:
			case INITIALIZING:
			case SHUTTING_DOWN:
				return true;
			default:
				return false;
		}
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}
