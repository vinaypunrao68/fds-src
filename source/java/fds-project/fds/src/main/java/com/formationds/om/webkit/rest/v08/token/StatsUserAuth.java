package com.formationds.om.webkit.rest.v08.token;

import java.util.Map;

import javax.crypto.SecretKey;

import org.eclipse.jetty.server.Request;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class StatsUserAuth implements RequestHandler {

	private Authenticator auth;
	private Authorizer authz;
	private SecretKey key;
	
	public StatsUserAuth( Authenticator auth, Authorizer authz, SecretKey key ){
		this.auth = auth;
		this.authz = authz;
		this.key = key;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		String username = request.getParameter( "username" );
		String password = request.getParameter( "password" );

		GrantToken authHandler = (new GrantToken( SingletonConfigAPI.instance().api(), getAuthenticator(), getAuthorizer(), getSecretKey() ) );
		
		JsonResource result = (JsonResource)authHandler.doLogin( username, password );
		
		if ( result.getHttpStatus() != 200 ){
			return new TextResource( "deny" );
		}
		
		String perms = "allow";
		
		if ( username.equals( "admin" ) ){
			perms += " administrator";
		}
		
		return new TextResource( perms );
		
	}
	
	private Authenticator getAuthenticator(){
		return auth;
	}
	
	private Authorizer getAuthorizer(){
		return authz;
	}
	
	private SecretKey getSecretKey(){
		return key;
	}

}
