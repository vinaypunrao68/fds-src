/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.security.HashedPassword;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.commons.io.IOUtils;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class UpdatePassword implements RequestHandler {
    
	private AuthenticationToken token;
    private ConfigurationApi configApi;
    private Authorizer authorizer;

    public UpdatePassword( Authorizer authorizer, 
                           AuthenticationToken token ) {
        this.token = token;
        this.authorizer = authorizer;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        
        String source = IOUtils.toString(request.getInputStream());
        JSONObject o = new JSONObject(source);
        
        String password = o.getString( "password" );
        
        User inputUser = ObjectModelHelper.toObject( source, User.class );
        
        com.formationds.apis.User currentUser = getAuthorizer().userFor( getToken() );

        return execute(currentUser, inputUser, password);
    }

    public Resource execute( com.formationds.apis.User currentUser, User inputUser, String password ) throws TException {
        
    	if (!currentUser.isFdsAdmin && inputUser.getId() != currentUser.getId()) {
            return new JsonResource(new JSONObject().put("message", "Access denied"), HttpServletResponse.SC_UNAUTHORIZED);
        }

        String hashedPassword = new HashedPassword().hash(password);

        com.formationds.apis.User iUser = getConfigApi().getUser( inputUser.getId() );
        
        getConfigApi().updateUser( iUser.getId(), iUser.getIdentifier(), hashedPassword, iUser.getSecret(), iUser.isIsFdsAdmin());
       
        User externalUser = ExternalModelConverter.convertToExternalUser( iUser );
        
        String jsonString = ObjectModelHelper.toJSON( externalUser );
        
        return new TextResource( jsonString );
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
    
    private Authorizer getAuthorizer(){
    	return this.authorizer;
    }
    
    private AuthenticationToken getToken(){
    	return this.token;
    }
}
