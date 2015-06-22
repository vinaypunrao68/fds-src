/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Tenant;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class ListTenants implements RequestHandler {

	private static final Logger logger = LoggerFactory
			.getLogger( ListTenants.class);
	
	private ConfigurationApi configApi;

    public ListTenants() {}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
    	
    	List<Tenant> tenants = Collections.emptyList();
    	
    	try {
    		tenants = listTenants();
    	}
    	catch( Exception e ){
    		logger.error( "Could not get a list of tenants in the system" );
    		logger.debug( "Exception: ", e );
    		throw e;
    	}
    	
    	String jsonString = ObjectModelHelper.toJSON( tenants );
    	
    	return new TextResource( jsonString );
    }
    
    public List<Tenant> listTenants() throws Exception{
    	
    	logger.debug( "Trying to get a list of tenants." );
    	
    	List<com.formationds.apis.Tenant> internalTenants = getConfigApi().listTenants( 0 );
    	
    	List<Tenant> externalTenants = new ArrayList<Tenant>();
    	
    	internalTenants.stream().forEach( iTenant -> {
    		Tenant externalTenant = ExternalModelConverter.convertToExternalTenant( iTenant );
    		externalTenants.add( externalTenant );
    	});
    	
    	return externalTenants;
    	
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
