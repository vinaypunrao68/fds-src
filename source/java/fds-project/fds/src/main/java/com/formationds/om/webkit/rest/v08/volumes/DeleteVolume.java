/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class DeleteVolume implements RequestHandler {

	private final static String VOLUME_ARG = "volume_id";
	private static final Logger logger = LoggerFactory.getLogger( DeleteVolume.class );
	private ConfigurationApi configApi;
	private Authorizer authorizer;
	private AuthenticationToken token;

	public DeleteVolume( Authorizer authorizer, AuthenticationToken token ) {
		
		this.authorizer = authorizer;
		this.token = token;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		long volumeId = requiredLong( routeParameters,  VOLUME_ARG );
		
		logger.debug( "Deleting volume: {}.", volumeId );

		String volumeName = getConfigApi().getVolumeName( volumeId );
		logger.debug( "Deleting volume: {}.", volumeId );
		if( volumeName == null || volumeName.length() <= 0 )
		{
			throw new ApiException( "The specified volume id ( " + volumeId + " ) wasn't found.",
									ErrorCode.MISSING_RESOURCE );
		}

		try
		{
			getAuthorizer().checkAccess( getToken(), "", volumeName );
			getConfigApi().deleteVolume( "", volumeName );
			return new JsonResource(new JSONObject().put("status", "ok"));
		}
		catch ( SecurityException e )
		{
			throw new ApiException( "Not authorized to delete volume id " + volumeId,
									ErrorCode.BAD_REQUEST );
		}
	}
	
	private Authorizer getAuthorizer(){
		return this.authorizer;
	}
	
	private AuthenticationToken getToken(){
		return this.token;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
