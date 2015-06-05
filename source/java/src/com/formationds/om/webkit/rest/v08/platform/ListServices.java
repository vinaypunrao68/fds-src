/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.Collections;
import java.util.List;
import java.util.Map;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;

import com.formationds.protocol.ApiException;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class ListServices implements RequestHandler{

	private static final String NODE_ARG = "node_id";
	private ConfigurationApi configApi;
	
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
		
		// TODO: Make a thrift call so we don't have to get all nodes and find the right one...
		List<Service> services = Collections.emptyList();
		
		Node node = (new GetNode()).getNode( nodeId );
		
		node.getServices().keySet().stream().forEach( serviceType -> {
			services.addAll( node.getServices().get( serviceType ) );
		});
		
		return services;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}
