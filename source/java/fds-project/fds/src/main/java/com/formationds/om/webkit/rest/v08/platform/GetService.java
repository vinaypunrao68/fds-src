package com.formationds.om.webkit.rest.v08.platform;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.client.v08.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetService implements RequestHandler{

	private static final Logger logger = LoggerFactory.getLogger( GetService.class );
	private static final String SERVICE_ARG = "service_id";
	private static final String NODE_ARG = "node_id";
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long nodeId = requiredLong( routeParameters, NODE_ARG );
		long serviceId = requiredLong( routeParameters, SERVICE_ARG );
		
		logger.debug( "Trying to retrieve service: " + serviceId + " running on node: " + nodeId );
		
		Service service = getService( nodeId, serviceId );
		
		String jsonString = ObjectModelHelper.toJSON( service );
		
		return new TextResource( jsonString );
	}
	
	public Service getService( long nodeId, long serviceId ) throws ApiException, TException {
		
		List<Service> services = (new ListServices()).getServicesForNode( nodeId );
		
		Optional<Service> foundService = services.stream().filter( service -> {
			if ( service.getId().equals( serviceId ) ){
				return true;
			}
			
			return false;
		}).findFirst();
		
		if ( foundService.isPresent() ){
			Service service = foundService.get();
			return service;
		}
		
		throw new ApiException( "Could not find service for node: " + nodeId + " and service: " + serviceId, ErrorCode.MISSING_RESOURCE );
	}

}
