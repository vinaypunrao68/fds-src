/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest.v08.token;

import com.formationds.apis.User;
import com.formationds.commons.model.type.Feature;
import com.formationds.commons.model.type.IdentityType;
import com.formationds.om.webkit.rest.HttpAuthenticator;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class GrantToken implements RequestHandler {
    private Authorizer authorizer;
    private Authenticator authenticator;
    private ConfigurationApi config;
    private SecretKey        key;

    public GrantToken(ConfigurationApi config, Authenticator auth, Authorizer authz, SecretKey key) {
        this.config = config;
        this.key = key;
        this.authorizer = authz;
        this.authenticator = auth;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(request, "login");
        String password = requiredString(request, "password");
        
        try {

        	return doLogin( login, password );
        	
        } catch (LoginException e) {
            JSONObject message = new JSONObject().put("message", "Invalid credentials.");
            return new JsonResource(message, HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
    
    /**
     * do the actual login in a way that cna be accessed by other classes
     * 
     * @param username
     * @param password
     * @return
     */
    public Resource doLogin( String username, String password ) throws LoginException{
       
        final List<String> features = new ArrayList<>();
        for (final Feature feature : Feature.byRole(IdentityType.USER)) {
            features.add(feature.name());
        }
    	
    	AuthenticationToken token = authenticator.authenticate(username, password);
        final User user = authorizer.userFor(token);
        if (user.isIsFdsAdmin()) {
            features.clear();
            for (final Feature feature : Feature.byRole(IdentityType.ADMIN)) {
                features.add( feature.name() );
          }
        }

        final JSONObject jsonObject = new JSONObject( );
        // temporary work-a-round for goldman
        jsonObject.put("username", username);
        jsonObject.put("userId", user.getId() );
        jsonObject.put("token", token.signature(key));
        jsonObject.put("features", features);
        // end of work-a-round

        // new JSONObject().put("token", token.signature(Authenticator.KEY))
        return new JsonResource(jsonObject) {
            @Override
            public Cookie[] cookies() {
                Cookie cookie = new Cookie( HttpAuthenticator.FDS_TOKEN, token.signature(key));
                cookie.setPath("/");
                return new Cookie[]{cookie};
            }
        };
    }
}
