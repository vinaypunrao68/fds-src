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

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class GetVolume  implements RequestHandler {
	
	public static final String VOLUME_ARG = "volume_id";
    private static final Logger logger = LoggerFactory.getLogger(ListVolumes.class);
    private Authorizer authorizer;
    private AuthenticationToken token;
   
    public GetVolume( Authorizer authorizer, AuthenticationToken token ){
    	this.authorizer = authorizer;
    	this.token = token;
    }
    
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
       
		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		
    	String showSysVolumeString = request.getParameter( ListVolumes.SHOW_SYSTEM_VOLUMES );
        Boolean showSysVolumes = Boolean.FALSE;
        
        if ( showSysVolumeString != null && showSysVolumeString.equalsIgnoreCase( "true" ) ){
        	showSysVolumes = Boolean.TRUE;
        }
		
		Volume volume = getVolume( volumeId, showSysVolumes );
		
		String jsonString = ObjectModelHelper.toJSON( volume );
		
		return new TextResource( jsonString );
	}
	
	public Volume getVolume( long volumeId ) throws Exception {
		return getVolume( volumeId, Boolean.FALSE );
	}
	
	public Volume getVolume( long volumeId, Boolean showSysVolumes ) throws Exception{
		
		logger.debug( "Retrieving volume: {}.", volumeId );
		
		List<Volume> volumes = (new ListVolumes( getAuthorizer(), getToken() )).listVolumes( showSysVolumes );
		
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
