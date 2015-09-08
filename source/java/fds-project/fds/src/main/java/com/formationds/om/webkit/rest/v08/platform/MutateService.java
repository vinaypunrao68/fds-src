/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.Service.ServiceState;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.pm.NotifyStopServiceMsg;
import com.formationds.protocol.pm.NotifyStartServiceMsg;
import com.formationds.protocol.pm.types.pmServiceStateTypeId;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;
import java.util.Map;
import java.util.ArrayList;

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

        logger.debug( "Trying to change service: " + serviceId + " on node: " + nodeId );
        
        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
        
        Service service = ObjectModelHelper.toObject( reader, Service.class );
        
        ServiceType type = service.getType();
        if ( (type == ServiceType.PM) || (type == ServiceType.OM) ) {
            throw new ApiException( "Cannot change state of service:" +  type +
        			                ". Valid types are: AM,DM,SM", ErrorCode.BAD_REQUEST);
        }
        
        service.setId( serviceId );
        
        Node node = (new GetNode()).getNode( nodeId );
        
        List<Service> curServices = node.getServices().get( type );
		
		if ( curServices == null || curServices.size() == 0 ){
            throw new ApiException( "Could not find service: " + type + "for node: "
                                    + nodeId + ", try adding first: ",
                                    ErrorCode.MISSING_RESOURCE );
		}
		
        Boolean amState = isServiceOnAlready( ServiceType.AM, node );
        Boolean smState = isServiceOnAlready( ServiceType.SM, node );
        Boolean dmState = isServiceOnAlready( ServiceType.DM, node );
        
        Boolean newState = convertState( service.getStatus().getServiceState() );
        
        // Get the service object so we can access the type
        Service svcObj = (new GetService()).getService(nodeId, serviceId);
        if (svcObj == null) {
        	throw new ApiException( "Could not retrieve valid service object",
        			                ErrorCode.MISSING_RESOURCE );
        }
        
        switch( svcObj.getType() ){
        	case AM:
        		if ( amState == newState) {
        		    throw new ApiException( "Service:AM is already in desired state",
        					                ErrorCode.BAD_REQUEST);
        		} else {
        		    amState = newState;
        		}
        		break;
        	case DM:
        		if ( dmState == newState ) {
        		    throw new ApiException( "Service:DM is already in desired state",
        					                ErrorCode.BAD_REQUEST);
        		} else {
        		    dmState = newState;
        		}
        		break;
        	case SM:
        		if ( smState == newState) {
        		    throw new ApiException( "Service:SM is already in desired state",
        					                ErrorCode.BAD_REQUEST);
        		} else {
        		    smState = newState;
        		}
        		break;
        	default:
        		break;
        }
        
        logger.debug( "Desired state of services is AM: " + amState +
        		      " SM: " + smState + " DM: " + dmState );

        List<SvcInfo> svcInfList = new ArrayList<SvcInfo>();
        SvcInfo svcInfo = PlatformModelConverter.convertServiceToSvcInfoType
        		                                 (node.getAddress().getHostAddress(),
        		                                  svcObj);
        svcInfList.add(svcInfo);
        
    	Service pmSvc = (new GetService()).getService(nodeId, nodeId);
    	SvcInfo pmSvcInfo = PlatformModelConverter.convertServiceToSvcInfoType
    			                                 (node.getAddress().getHostAddress(),
                                                  pmSvc);
    	svcInfList.add(pmSvcInfo);
    	
        int status = -1;
        
        if ( newState ) {
        	// State change desired is from NOT_RUNNING to RUNNING
        	status = getConfigApi().StartService(new NotifyStartServiceMsg(svcInfList));
        }
        else
        {   // State change desired is from RUNNING to NOT_RUNNING
        	status = getConfigApi().StopService(new NotifyStopServiceMsg(svcInfList, false));
        }

        if ( status != 0 ){
        	
            status = HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE_ERROR,
                                      nodeId );
            
            throw new ApiException( "Service state change failed.", ErrorCode.INTERNAL_SERVER_ERROR );
        }
        else {
            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE,
                    nodeId );
        }
        
        Service currentService = (new GetService()).getService( nodeId, serviceId );
        
        String jsonString = ObjectModelHelper.toJSON( currentService );
	       
        return new TextResource( jsonString );
	}
	
	
	/**
	 * Find the current condition of the AM, SM and DM service
	 */
	private Boolean isServiceOnAlready( ServiceType type, Node node ){
		
		List<Service> services = node.getServices().get( type );
		
		if ( services == null || services.size() == 0 ){
			return false;
		}
		
		Service service = services.get( 0 );
		
		return convertState( service.getStatus().getServiceState() );
	}
	
	private Boolean convertState( ServiceState status ){
		
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
