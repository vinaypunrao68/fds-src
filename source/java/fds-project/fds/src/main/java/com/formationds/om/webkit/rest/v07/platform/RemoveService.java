package com.formationds.om.webkit.rest.v07.platform;

import java.util.Iterator;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.Service;
import com.formationds.om.events.EventManager;
import com.formationds.om.events.OmEvents;
import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class RemoveService implements RequestHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( RemoveService.class );

    private ConfigurationService.Iface client;
	
	public RemoveService( final ConfigurationService.Iface client ){
		
		this.client = client;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		Long nodeId = requiredLong( routeParameters, "node_uuid" );
		Long serviceId = requiredLong(routeParameters, "service_uuid");

		logger.debug("Removing service {}", serviceId );
		
		//TODO:  This is silly.  First of all I shouldn't need the node ID if I have the service ID.
		//  Secondly, I shouldn't need the node name if I have the ID!  Now I need to loop through 
		// nodes to find a name I shouldn't need.
        List<com.formationds.protocol.svc.types.FDSP_Node_Info_Type> list =
                client.ListServices( 0 );
        
        Map<String, Node> nodeMap = (new ListNodes(client)).computeNodeMap(list);
		
		Node myNode = nodeMap.get( nodeId.toString() );
		int status = 1;
		int httpCode = HttpServletResponse.SC_NOT_FOUND;
		
		if ( myNode != null ){
			
			Service service = findService( myNode, serviceId );
			
			if ( service != null ){
				
				// find out what type it is
				Boolean am = false;
				Boolean dm = false;
				Boolean sm = false;
				
				switch( service.getType() ){
					case FDSP_ACCESS_MGR:
						am = true;
						break;
					case FDSP_DATA_MGR:
						dm = true;
						break;
					case FDSP_STOR_MGR:
						sm = true;
						break;
					default:
						break;
				}
				
				FDSP_Uuid uuid = new FDSP_Uuid( nodeId );
				
				status =  
						client.RemoveServices( new FDSP_RemoveServicesType(
							myNode.getName(), 
							uuid, 
							sm, 
							dm, 
							am ));
				
				httpCode = HttpServletResponse.SC_OK;

			}
			else {
				logger.warn( "Could not locate the specified service on the specified node.  Remove service failed." );
			}
		}
		else {
			logger.warn( "Could not locate the specified node.  The ID " + nodeId + " does not exist." );
		}
        
		if (status != 0) {

			status = HttpServletResponse.SC_BAD_REQUEST;
			EventManager.notifyEvent(OmEvents.REMOVE_SERVICE_ERROR, serviceId );
			logger.error( "An error occurred while trying to removed service: " + serviceId );

		} else {

			EventManager.notifyEvent(OmEvents.REMOVE_SERVICE, serviceId);
			logger.info( "Service " + serviceId + " was successfully removed." );

		}					

		return new JsonResource(new JSONObject().put("status", status),
				httpCode);
	}
	
	/**
	 * Find the service in the node
	 * @param node
	 * @return
	 */
	private Service findService( Node node, Long serviceId ){
		
		Iterator<List<Service>> serviceListsIt = node.getServices().values().iterator();
		
		while( serviceListsIt.hasNext() ){
			
			Iterator<Service> serviceIt = serviceListsIt.next().iterator();
			
			while( serviceIt.hasNext() ){
				
				Service service = serviceIt.next();
				
				if ( service.getUuid().equals( serviceId ) ){
					return service;
				}
			}// while service
		}// while service list
		
		return null;
	}

}
