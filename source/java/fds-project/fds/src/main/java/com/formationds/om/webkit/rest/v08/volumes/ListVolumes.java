/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ListVolumes implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(ListVolumes.class);

    private ConfigurationApi configApi;
    private Authorizer authorizer;
    private AuthenticationToken token;
    
    public ListVolumes( Authorizer authorizer, AuthenticationToken token ){
    	this.authorizer = authorizer;
    	this.token = token;
    }
    
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
       
		List<Volume> externalVolumes = listVolumes();
		
		String jsonString = ObjectModelHelper.toJSON( externalVolumes );
		
		return new TextResource( jsonString );
	}
	
	public List<Volume> listVolumes() throws Exception{
		
		String domain = "";
		List<VolumeDescriptor> rawVolumes = getConfigApi().listVolumes( domain );
		
		// filter the results to things you can see and get rid of system volumes
		rawVolumes = rawVolumes.stream()
			// TODO: Fix the HACK!  Should have an actual system volume "type" that we can check
			.filter( descriptor -> {
				LOG.debug( "Removing volume " + descriptor.getName() + " from the volume list." );
				Boolean systemVolume = descriptor.getName().startsWith( "SYSTEM_VOLUME" );
				return !systemVolume;
			})
			.filter( descriptor -> {
				LOG.debug( "Removing a volume that the caller does not have access to." );
				Boolean owns = getAuthorizer().ownsVolume( getToken(), descriptor.getName() );
				return owns;
			})
			.collect( Collectors.toList() );
		
		List<Volume> externalVolumes = new ArrayList<>();
		
		rawVolumes.stream().forEach( descriptor ->{
			
			LOG.trace( "Converting volume " + descriptor.getName() + " to external format." );
			Volume externalVolume = ExternalModelConverter.convertToExternalVolume( descriptor );
			externalVolumes.add( externalVolume );
		});		
		
		return externalVolumes;
	}
	
	private Authorizer getAuthorizer(){
		return this.authorizer;
	}
	
	private AuthenticationToken getToken(){
		return token;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}