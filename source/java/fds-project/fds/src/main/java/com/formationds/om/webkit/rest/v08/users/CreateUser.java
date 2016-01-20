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

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;
import java.util.UUID;

public class CreateUser implements RequestHandler {

    private static final Logger logger = LoggerFactory.getLogger( CreateUser.class );
    private ConfigurationApi configApi;

    public CreateUser() {}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

        String source = IOUtils.toString(request.getInputStream());
        JSONObject o = new JSONObject(source);

        User inputUser = ObjectModelHelper.toObject( source, User.class );

        logger.info( "Create user: {}.", inputUser.getName() );

        String login = inputUser.getName();
        String password = o.getString( "password" );

        String hashed = new HashedPassword().hash(password);
        String secret = UUID.randomUUID().toString();
        long id = getConfigApi().createUser(login, hashed, secret, false);

        com.formationds.apis.User internalUser = getConfigApi().getUser( id );

        User externalUser = ExternalModelConverter.convertToExternalUser( internalUser );

        if ( inputUser.getTenant() != null ){
            getConfigApi().assignUserToTenant( externalUser.getId(), inputUser.getTenant().getId() );
        }

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
