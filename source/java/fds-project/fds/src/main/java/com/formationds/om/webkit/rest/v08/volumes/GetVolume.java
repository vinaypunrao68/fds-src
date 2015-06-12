/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import java.util.List;
import java.util.Map;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

public class GetVolume  implements RequestHandler {
	
	public static final String VOLUME_ARG = "volume_id";
    private static final Logger LOG = Logger.getLogger(ListVolumes.class);

    private Authorizer authorizer;
    private AuthenticationToken token;
   
    public GetVolume( Authorizer authorizer, AuthenticationToken token ){
    	this.authorizer = authorizer;
    	this.token = token;
    }
    
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
       
		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		
		Volume volume = getVolume( volumeId );
		
		String jsonString = ObjectModelHelper.toJSON( volume );
		
		return new TextResource( jsonString );
	}
	
	public Volume getVolume( long volumeId ) throws Exception{
		
		List<Volume> volumes = (new ListVolumes( getAuthorizer(), getToken() )).listVolumes();
		
		for ( Volume volume : volumes ){
			if ( volume.getId().equals( volumeId ) ){
				return volume;
			}
		}
		
		throw new ApiException( "Volume cound not be located", ErrorCode.MISSING_RESOURCE );
	}
	
	private Authorizer getAuthorizer(){
		return this.authorizer;
	}
	
	private AuthenticationToken getToken(){
		return this.token;
	}
}
