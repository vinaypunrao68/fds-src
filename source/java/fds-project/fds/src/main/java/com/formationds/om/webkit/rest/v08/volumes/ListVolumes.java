/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeType;
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

import org.eclipse.jetty.server.Request;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ListVolumes implements RequestHandler {
    private static final Logger logger = LogManager.getLogger( ListVolumes.class );
    
    public static final String SHOW_SYSTEM_VOLUMES = "system-volumes"; 

    private ConfigurationApi    configApi;
    private Authorizer          authorizer;
    private AuthenticationToken token;

    public ListVolumes( Authorizer authorizer, AuthenticationToken token ) {
        this.authorizer = authorizer;
        this.token = token;
    }

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters ) throws Exception {

    	String showSysVolumeString = request.getParameter( SHOW_SYSTEM_VOLUMES );
        Boolean showSysVolumes = Boolean.FALSE;
        
        if ( showSysVolumeString != null && showSysVolumeString.equalsIgnoreCase( "true" ) ){
        	showSysVolumes = Boolean.TRUE;
        }
    	
        List<Volume> externalVolumes = listVolumes( showSysVolumes );

        String jsonString = ObjectModelHelper.toJSON( externalVolumes );
        
        return new TextResource( jsonString );
    }
    
    public List<Volume> listVolumes() throws Exception {
    	return listVolumes( Boolean.FALSE );
    }

    public List<Volume> listVolumes( Boolean showSysVolumes ) throws Exception {

        logger.debug( "Listing all volumes." );

        final Boolean showSys;
        
        // only admins can look at system volumes
        User user = getAuthorizer().userFor( getToken() );
        if ( !getAuthorizer().userFor( getToken() ).isIsFdsAdmin() ){
        	logger.warn( "Unauthorized access to system volumes attempted by user: " + user.id );
        	showSys = Boolean.FALSE;
        }
        else {
        	showSys = showSysVolumes;
        }
        
        String domain = "";
        List<VolumeDescriptor> rawVolumes = getConfigApi().listVolumes( domain );

        // filter the results to things you can see and get rid of system volumes
        rawVolumes = rawVolumes.stream()
                               .filter( descriptor -> {
                                   // TODO: Fix the HACK!  Should have an actual system volume "type" that we can check
                                   boolean sysvol = descriptor.getName().startsWith( "SYSTEM_" );
                                   if ( sysvol && !showSys ) {
                                       logger.debug( "Removing volume " + descriptor.getName() +
                                                     " from the volume list." );
                                       return Boolean.FALSE;
                                   }
                                   else {
                                	   return Boolean.TRUE;
                                   }

                               } )
                               .filter( descriptor -> {
                                   boolean hasAccess = getAuthorizer().ownsVolume( getToken(), descriptor.getName() );
                                   if ( !hasAccess ) {
                                       logger.debug( "Removing a volume that the caller does not have access to." );
                                   }
                                   return hasAccess;
                               } )
                               .collect( Collectors.toList() );

        List<Volume> externalVolumes = ExternalModelConverter.convertToExternalVolumes( rawVolumes );

        logger.debug( "Found {} volumes.", externalVolumes.size() );

        return externalVolumes;
    }

    private Authorizer getAuthorizer() {
        return this.authorizer;
    }

    private AuthenticationToken getToken() {
        return token;
    }

    protected ConfigurationApi getConfigApi() {

        if ( configApi == null ) {
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }
}
