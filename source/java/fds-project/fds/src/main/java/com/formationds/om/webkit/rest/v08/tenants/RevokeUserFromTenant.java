/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 *
 */
package com.formationds.om.webkit.rest.v08.tenants;

import java.util.Map;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class RevokeUserFromTenant implements RequestHandler{

	private final static String TENANT_ARG = "tenant_id";
	private final static String USER_ARG = "user_id";
	
	private ConfigurationApi configApi;

	public RevokeUserFromTenant() {}

	@Override
	public Resource handle(Request request,
						   Map<String, String> routeParameters) throws Exception {

		long tenantId = requiredLong( routeParameters, TENANT_ARG );
		long userId = requiredLong( routeParameters, USER_ARG );
		
		getConfigApi().revokeUserFromTenant(userId, tenantId);
		
		return new JsonResource(new JSONObject().put("status", "ok"));
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
