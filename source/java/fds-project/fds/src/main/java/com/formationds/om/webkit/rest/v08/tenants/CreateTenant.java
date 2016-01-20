/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Tenant;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

public class CreateTenant implements RequestHandler {

	private static final Logger logger = LoggerFactory
			.getLogger( CreateTenant.class);

    private ConfigurationApi configApi;

    public CreateTenant() {}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

        HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );

        Tenant tenant = null;
    	try ( final InputStreamReader reader = new InputStreamReader( requestLoggingProxy.getInputStream() ) ) {
            tenant = ObjectModelHelper.toObject( reader, Tenant.class );
    	}

    	logger.debug( "Trying to create tenant: " + tenant.getName() );

    	long tenantId = getConfigApi().createTenant( tenant.getName() );

    	List<com.formationds.apis.Tenant> internalTenants = getConfigApi().listTenants(0);
    	Tenant newTenant = null;

    	for ( com.formationds.apis.Tenant internalTenant : internalTenants ){

    		if ( internalTenant.getId() == tenantId ){
    			newTenant = ExternalModelConverter.convertToExternalTenant( internalTenant );
    			break;
    		}
    	}

    	String jsonString = ObjectModelHelper.toJSON( newTenant );

    	return new TextResource( jsonString );
    }

    private ConfigurationApi getConfigApi(){

    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}

    	return configApi;
    }
}
