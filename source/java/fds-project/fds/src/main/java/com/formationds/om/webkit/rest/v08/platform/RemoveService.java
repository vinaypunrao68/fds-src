/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.FDSP_RemoveServicesType;
import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Service;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.FDSP_Uuid;
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
		
		Node node = (new GetNode()).getNode( nodeId );
		
		// TODO: Yes, this is redundant but it's easy.  We should fix the thrift part of this insanity
		// instead of focusing on this.
		Service service = (new GetService()).getService( nodeId, serviceId );
		
		Boolean am = false;
		Boolean sm = false;
		Boolean dm = false;
		
		switch( service.getType() ){
			case AM:
				am = true;
				break;
			case DM:
				dm = true;
				break;
			case SM:
				sm = true;
				break;
			default:
				break;
		}
		
		getConfigApi().RemoveServices( new FDSP_RemoveServicesType(
				node.getName(),
				new FDSP_Uuid( node.getId() ),
				sm,
				dm, 
				am) );
		
		return new JsonResource( new JSONObject().put("status", "ok"), HttpServletResponse.SC_OK );
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
