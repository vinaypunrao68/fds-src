package com.formationds.om.webkit.rest.v08.token;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.model.User;
import com.formationds.om.webkit.rest.v08.users.GetUser;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class StatsResourceAuth implements RequestHandler {

	private static final Logger logger = LoggerFactory.getLogger( StatsResourceAuth.class );
	
	private static final String READ = "read";
	private static final String CONFIGURE = "configure";
	private static final String WRITE = "write";
	private static final String QUEUE = "queue";
	private static final String EXCHANGE = "exchange";
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		String username = request.getParameter( "username" );
		String resource = request.getParameter( "resource" );
		String name = request.getParameter( "name" );
		String permission = request.getParameter( "permission" );
		
		List<String> permissionSet = establishPermissionSet( username, resource, name );
		String result = "deny";
		
		for ( String realPerm : permissionSet ){
			
			if ( realPerm.equals( permission ) ){
				result = "allow";
				break;
			}
		}
		
		return new TextResource( result );
	}
	
	private List<String> establishPermissionSet( String username, String resource, String name ){
		
		List<String> perms = Collections.emptyList();
		
		if ( username.equals( "stats-service" ) ){
			perms = getStatsServicePerms( resource, name );
		}
		else if ( username.equals( "admin" ) ){
			perms = getAdminPerms( resource, name );
		}
		else {
			perms = getUserPerms( username, resource, name );
		}
		
		return perms;
	}
	
	private List<String> getUserPerms( String username, String resource, String name ){
		
		List<String> perms = new ArrayList<String>();
		
		// It only gets to the queue level if the exchange level
		// has been authorized.  We will authorize exchange based on 
		// tenancy and the queue must be read writable
		if ( resource.equals( QUEUE ) ){
			perms.add( WRITE );
			perms.add( CONFIGURE );
			perms.add( READ );
			return perms;
		}
		
		Long tenantId = -1L;
		
		String[] tokens = name.split( "." );
		
		if ( tokens.length > 1 ){
			String tenantIdStr = tokens[1];
			
			try {
				tenantId = Long.parseLong( tenantIdStr );
			}
			catch( NumberFormatException ne ){
				logger.warn( "Unfamiliar message topic format: " + name + " - No permisisons granted.");
			}
		}
		
		User user = (new GetUser()).getUser(username );
		
		if ( tenantId.equals( user.getTenant().getId() ) ){
			perms.add( READ );
		}
		
		return perms;
	}
	
	private List<String> getStatsServicePerms( String resource, String name ){
		
		List<String> perms = new ArrayList<String>();
		
		if ( resource.equals( QUEUE ) ) {

			if ( name.equals( "stats.work" ) || name.equals( "stats.query" ) ) {
				
				perms.add( READ );
				perms.add( CONFIGURE );
				
			} else if ( name.equals( "stats.aggregation" ) ) {
				
				perms.add( READ );
				perms.add( WRITE );
				perms.add( CONFIGURE );
			}
		}
		else {
			perms.add( WRITE );
			perms.add( CONFIGURE );
		}
		
		return perms;
	}
	
	private List<String> getAdminPerms( String resource, String name ){
		List<String> perms = new ArrayList<String>();
		
		if ( resource.equals( EXCHANGE ) ){
			
			perms.add( READ );
			perms.add( WRITE );
			perms.add( CONFIGURE );
		}
		else {
			if ( name.equals( "stats.work" ) ){
				perms.add( WRITE );
				perms.add( CONFIGURE );
			} 
			else {
				perms.add( CONFIGURE );
				perms.add( READ );
				perms.add( WRITE );
			}
		}
		
		return perms;
	}

}
