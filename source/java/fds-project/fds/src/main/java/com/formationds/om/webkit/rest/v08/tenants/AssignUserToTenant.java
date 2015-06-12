/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class AssignUserToTenant implements RequestHandler {
	
	private final static Logger logger = LoggerFactory.getLogger( AssignUserToTenant.class );
	private final static String TENANT_ARG = "tenant_id";
	private final static String USER_ARG = "user_id";
	
    private ConfigurationApi configApi;

    public AssignUserToTenant(){}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

    	long userId = requiredLong( routeParameters, USER_ARG );
    	long tenantId = requiredLong( routeParameters, TENANT_ARG );
    	
    	logger.debug( "Assigning user: {} to tenant: {}.", userId, tenantId );
    	
    	getConfigApi().assignUserToTenant( userId, tenantId );
    	
        return new JsonResource(new JSONObject().put("status", "ok"));
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
