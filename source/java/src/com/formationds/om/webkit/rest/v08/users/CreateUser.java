/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.HashedPassword;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

import java.util.Map;
import java.util.UUID;

public class CreateUser implements RequestHandler {

	private ConfigurationApi configApi;
    
    private static final String LOGIN_KEY = "login";
    private static final String PASSWORD_KEY = "password";

    public CreateUser() {}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        
    	String login = requiredString( routeParameters, LOGIN_KEY );
        String password = requiredString( routeParameters, PASSWORD_KEY );

        String hashed = new HashedPassword().hash(password);
        String secret = UUID.randomUUID().toString();
        long id = getConfigApi().createUser(login, hashed, secret, false);
        
        com.formationds.apis.User internalUser = getConfigApi().getUser( id );
        
        User externalUser = ExternalModelConverter.convertToExternalUser( internalUser );
        
        String jsonString = ObjectModelHelper.toJSON( externalUser );
        
        return new TextResource( jsonString );
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
