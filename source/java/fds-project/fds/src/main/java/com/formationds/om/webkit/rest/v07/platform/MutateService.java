package com.formationds.om.webkit.rest.v07.platform;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.FDSP_ActivateOneNodeType;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.ServiceStatus;
import com.formationds.commons.model.type.ServiceType;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class MutateService implements RequestHandler {

    private static final Logger logger = LoggerFactory.getLogger( MutateService.class );

    private ConfigurationService.Iface client;
	
	public MutateService( final ConfigurationService.Iface client ){
		
		this.client = client;
	}
	
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
		
        Long nodeUuid = requiredLong(routeParameters, "node_uuid");        
        Long serviceId = requiredLong(routeParameters, "service_uuid");

        final Reader reader = new InputStreamReader( request.getInputStream(), "UTF-8" );
       
        List<com.formationds.protocol.svc.types.FDSP_Node_Info_Type> list = client.ListServices( 0 );
        
        ListNodes listNodes = new ListNodes( client );
        Map<String, Node> nodes = listNodes.computeNodeMap( list );
        
        Node ourNode = nodes.get( nodeUuid.toString() );
        
        int status = 0;
        int httpCode = HttpServletResponse.SC_INTERNAL_SERVER_ERROR;
        
        if ( ourNode == null ){
        	logger.warn( "Could not find a Node with ID: " + nodeUuid );
        	return sendReturn( 0, HttpServletResponse.SC_BAD_REQUEST, nodeUuid );
        } 
        	
        Boolean startAm = true;
        Boolean startDm = true;
        Boolean startSm = true;
        
        List<Service> ams = ourNode.getServices().get( ServiceType.AM );
        List<Service> sms = ourNode.getServices().get( ServiceType.DM );
        List<Service> dms = ourNode.getServices().get( ServiceType.SM );
        
        Service myService = findMyService( ourNode, serviceId );
        Service argService = ObjectModelHelper.toObject( reader, myService.getClass() );
        Boolean startService = (argService.getStatus().equals( ServiceStatus.ACTIVE ));
        
        //TODO: Now, because of how the thrift call works, we only look at the first one in each list.
        startAm = shouldIStartService(  ams );
        startSm = shouldIStartService(  sms );
        startDm = shouldIStartService(  dms );
        
        // now... figure out what we were and overwrite that value with the passed in state.
        switch( myService.getType() ){
        	case FDSP_ACCESS_MGR:
        		startAm = startService;
        		break;
        	case FDSP_DATA_MGR:
        		startDm = startService;
        		break;
        	case FDSP_STOR_MGR:
        		startSm = startService;
        		break;
        	default:
        		break;
        }
        
        status = 
        		client.ActivateNode( new FDSP_ActivateOneNodeType(
                                     1,
                                     new FDSP_Uuid( nodeUuid ),
                                     startSm,
                                     startDm,
                                     startAm ) );
        
        httpCode = HttpServletResponse.SC_OK;
	       
        return sendReturn(status, httpCode, nodeUuid);
	}
	
	private JsonResource sendReturn( int status, int httpCode, Long nodeUuid ){
        
		if( status != 0 ) {

            status= HttpServletResponse.SC_BAD_REQUEST;
            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE_ERROR,
                                      nodeUuid );
            logger.error( "Service state changed failed." );

        } else {

            EventManager.notifyEvent( OmEvents.CHANGE_SERVICE_STATE,
                                      nodeUuid );
            logger.info( "Service state changed successfully." );

        }

        return new JsonResource( new JSONObject().put( "status", status ),
                                 httpCode); 		
	}
	
	/**
	 * Go through the node and find the service we were looking for
	 * @param node
	 * @param serviceId
	 * @return Service - The matching one
	 */
	private Service findMyService( Node node, Long serviceId ){
		
		Iterator<List<Service>> serviceListIt = node.getServices().values().iterator();
		
		while ( serviceListIt.hasNext() ){
			List<Service> serviceList = serviceListIt.next();
			Iterator<Service> serviceIt = serviceList.iterator();
			
			while( serviceIt.hasNext() ){
				Service service = serviceIt.next();
				
				if ( service.getUuid().equals( serviceId ) ){
					return service;
				}
			}
		}
		
		return null;
	}
	
	/**
	 * Find the current condition of the AM, SM and DM service
	 * 
	 * @param ams
	 * @return
	 */
	private Boolean shouldIStartService( List<Service> ams ){
		
		if ( ams.isEmpty() ){
			return false;
		}
		
		Service amService = ams.get( 0 );
		
		switch( amService.getStatus() ){
			case ACTIVE:
				return true;
			case ERROR:
			case INACTIVE:
			case INVALID:
			default:
				return false;
		}
	}

}
