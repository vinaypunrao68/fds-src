/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListUsers implements RequestHandler {
	
	private static final Logger logger = LoggerFactory
			.getLogger(ListUsers.class);
	
    private ConfigurationApi configApi;

    public ListUsers() {}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
    	
    	List<User> externalUsers = null;
    	
    	try{
    		externalUsers = listUsers();
    	}
    	catch( Exception e ){
    		logger.error( "Could not obtain a list of users." );
    		logger.debug( "Caused by:", e );
    		throw e;
    	}
    	
    	String jsonString = ObjectModelHelper.toJSON( externalUsers );
    	
        return new TextResource( jsonString );
    }
    
    public List<User> listUsers() throws ApiException, TException{
    	
    	logger.debug( "Trying to list users." );
    	
    	List<User> externalUsers = new ArrayList<>();
    	
    	getConfigApi().allUsers( 0 ).stream().forEach( user -> {
    		
    		User externalUser = ExternalModelConverter.convertToExternalUser( user );
    		
    		if ( externalUser.getName().equalsIgnoreCase( GetUser.STATS_USERNAME ) ){
    			logger.debug( "Removing stats user from the list." );
    			return;
    		}
    		
    		externalUsers.add( externalUser );
    	});
    	
    	return externalUsers;
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
