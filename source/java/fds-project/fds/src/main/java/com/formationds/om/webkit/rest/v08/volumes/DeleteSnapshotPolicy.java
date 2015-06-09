/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import java.util.Map;

import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class DeleteSnapshotPolicy implements RequestHandler{

	private static final String VOLUME_ARG = "volume_id";
	private static final String POLICY_ARG = "policy_id";
	
	private ConfigurationApi configApi;
	
	public DeleteSnapshotPolicy(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		long policyId = requiredLong( routeParameters, POLICY_ARG );
		
		getConfigApi().detachSnapshotPolicy( volumeId, policyId );
		
		getConfigApi().deleteSnapshotPolicy( policyId );
		
		return new JsonResource( new JSONObject().put( "status", "ok"), HttpServletResponse.SC_OK );
	}
	
	public ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}
