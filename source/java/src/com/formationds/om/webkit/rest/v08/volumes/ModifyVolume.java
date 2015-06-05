/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import java.io.InputStreamReader;
import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class ModifyVolume implements RequestHandler{

	private static final String VOLUME_ARG = "volume_id";
	
	private ConfigurationApi configApi;
	private Authorizer authorizer;
	private AuthenticationToken token;
	
	public ModifyVolume( Authorizer authorizer, AuthenticationToken token){
		
		this.authorizer = authorizer;
		this.token = token;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		
		final InputStreamReader reader = new InputStreamReader( request.getInputStream() );
		Volume volume = ObjectModelHelper.toObject( reader, Volume.class );
		volume.setId( volumeId );
		
		(new CreateVolume( getAuthorizer(), getToken() )).setQosForVolume( volume );
		
		Volume newVolume = (new GetVolume( getAuthorizer(), getToken() ) ).getVolume( volumeId );
		
		String jsonString = ObjectModelHelper.toJSON( newVolume );
		
		return new TextResource( jsonString );
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
