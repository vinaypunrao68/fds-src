package com.formationds.om.webkit.rest.v08.users;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.User;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class ListUsersForTenant implements RequestHandler{

	private static final Logger logger = LoggerFactory
			.getLogger(ListUsersForTenant.class);
	
	private final static String TENANT_ARG = "tenant_id";
	
	private ConfigurationApi configApi;
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long tenantId = requiredLong( routeParameters, TENANT_ARG );
		
		logger.debug( "Finding users for tenant: {}.", tenantId );
		
		List<User> users = Collections.emptyList();
		
		try {
			users = listUsersForTenant( tenantId );
		}
		catch( Exception e ){
			logger.error( "Could not list users for tenant: " + tenantId );
			logger.debug( "Exception:", e );
			throw e;
		}
		
		String jsonString = ObjectModelHelper.toJSON( users );
		
		return new TextResource( jsonString );
		
	}
	
	private List<User> listUsersForTenant( long tenantId ) throws Exception {
		
		List<com.formationds.apis.User> internalUsers = getConfigApi().listUsersForTenant( tenantId );
		
		List<User> externalUsers = new ArrayList<>();
		
		internalUsers.stream().forEach( internalUser -> {
			User externalUser = ExternalModelConverter.convertToExternalUser( internalUser );
			
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
