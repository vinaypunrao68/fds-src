package com.formationds.om.webkit.rest.v08.token;

import java.io.ByteArrayOutputStream;
import java.util.Map;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.webkit.rest.v08.ApiDefinition;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.spike.later.HttpContext;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class StatsUserAuth implements RequestHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( StatsUserAuth.class );
	
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

		if ( username.equals( "stats-service" ) && password.equals( "$t@t$" ) ){
			return new TextResource( "allow" );
		}
		
		// Check if the password is a token first
		AuthenticationToken token = null;
		
		try {
			token = getAuthenticator().parseToken( password );
		}
		catch ( LoginException le ){
			logger.info( "Access key was not a token - attempting as credentials" );
		}
		
		if ( token == null ){
			GrantToken authHandler = (new GrantToken( SingletonConfigAPI.instance().api(), getAuthenticator(), getAuthorizer(), getSecretKey() ) );
			JsonResource result = (JsonResource)authHandler.doLogin( username, password );
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			result.render( baos );
			
			JSONObject obj = new JSONObject( baos.toString() );
			
			try{
				token = getAuthenticator().parseToken( obj.getString( "token" ) );
			}
			catch ( LoginException le ){
				logger.info( "Credential authentication failed as well." );
			}
		}
		
		
		if ( token == null ){
			logger.warn( "FDS bus access denied." );
			return new TextResource( "deny" );
		}
		
		String perms = "allow";
		
		if ( getAuthorizer().userFor( token ).isIsFdsAdmin() ){
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
