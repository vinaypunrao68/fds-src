/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CurrentUser implements RequestHandler {
    private Authorizer  authz;
    private AuthenticationToken token;

    public CurrentUser(Authorizer authz, AuthenticationToken token) {
        this.authz = authz;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
    	
    	com.formationds.apis.User internalUser = getAuthorizer().userFor( getToken() );
    	User myUser = ExternalModelConverter.convertToExternalUser( internalUser );
    	
    	String jsonString = ObjectModelHelper.toJSON( myUser );
    	
    	return new TextResource( jsonString );
    }
    
    private Authorizer getAuthorizer(){
    	return this.authz;
    }
    
    private AuthenticationToken getToken(){
    	return this.token;
    }
}
