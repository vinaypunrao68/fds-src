/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.security.HashedPassword;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

import javax.servlet.http.HttpServletResponse;

public class UpdatePassword implements RequestHandler {

    private static final String USER_ARG = "user_id";

    private static final Logger logger = LoggerFactory.getLogger( UpdatePassword.class );
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

        long userId = requiredLong( routeParameters, USER_ARG );
        String password = o.getString( "password" );

        logger.info( "Changing password for user: {}.", userId );

        User inputUser = ObjectModelHelper.toObject( source, User.class );

        try {
            o.getString( "id" );
        }
        // no id... let's make one.
        catch( JSONException jException ){
            inputUser = new User( userId, inputUser.getName(), inputUser.getRoleId(), inputUser.getTenant() );
        }

        com.formationds.apis.User currentUser = getAuthorizer().userFor( getToken() );

        return execute(currentUser, inputUser, password);
    }

    public Resource execute( com.formationds.apis.User currentUser, User inputUser, String password ) throws TException {

        if ( !currentUser.isIsFdsAdmin() && inputUser.getId() != currentUser.getId()) {
            throw new ApiException( "Access denied.", ErrorCode.INTERNAL_SERVER_ERROR );
        }

        if ( inputUser.getName().equalsIgnoreCase( GetUser.STATS_USERNAME ) ){
            throw new ApiException( "Unable to access user for modification.", ErrorCode.INTERNAL_SERVER_ERROR );
        }

        String hashedPassword = new HashedPassword().hash(password);

        com.formationds.apis.User iUser = getConfigApi().getUser( inputUser.getId() );

        getConfigApi().updateUser( iUser.getId(), iUser.getIdentifier(), hashedPassword, iUser.getSecret(), iUser.isIsFdsAdmin());

        return new JsonResource( (new JSONObject()).put( "status", "ok" ), HttpServletResponse.SC_OK );
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
