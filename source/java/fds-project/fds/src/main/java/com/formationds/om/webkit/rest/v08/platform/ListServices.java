/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.protocol.ApiException;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class ListServices implements RequestHandler{

	private static final Logger logger = LoggerFactory.getLogger( ListServices.class );
	private static final String NODE_ARG = "node_id";
	
	public ListServices(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long nodeId = requiredLong( routeParameters, NODE_ARG );
		
		List<Service> services = getServicesForNode( nodeId );
		
		String jsonString = ObjectModelHelper.toJSON( services );
		
		return new TextResource( jsonString );
	}
	
	public List<Service> getServicesForNode( long nodeId ) throws ApiException, TException {
		
		logger.debug( "Listing service for node: " + nodeId );
		
		// TODO: Make a thrift call so we don't have to get all nodes and find the right one...
		List<Service> services = new ArrayList<>();
		
		Node node = (new GetNode()).getNode( nodeId );
		
		node.getServices().keySet().stream().forEach( serviceType -> {
			services.addAll( node.getServices().get( serviceType ) );
		});
		
		logger.debug( "Found " + services.size() + " services." );
		
		return services;
	}
}
