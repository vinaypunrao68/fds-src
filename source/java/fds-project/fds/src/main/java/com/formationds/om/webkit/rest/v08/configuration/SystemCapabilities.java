/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.configuration;

import com.formationds.client.v08.model.MediaPolicy;
import com.formationds.commons.model.SystemCapability;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.dcache.nfs.v4.xdr.newoffset4;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class SystemCapabilities
    implements RequestHandler {

	private Logger logger = LoggerFactory.getLogger( SystemCapabilities.class );
	
    private final ParsedConfig parsedConfig;
    
    private final String TOGGLES = "toggles";

    public SystemCapabilities( final ParsedConfig parsedConfig ) {

        this.parsedConfig = parsedConfig;

    }


    @Override
    public Resource handle( final Request request, final Map<String, String> routeParameters )
        throws Exception {

    	logger.debug( "Requesting system capabilities" );
    	
        final List<String> mediaPolicies = new ArrayList<>( );
        mediaPolicies.add( MediaPolicy.SSD.name() );
        if( !parsedConfig.defaultBoolean( "fds.feature_toggle.common.all_ssd",
                                          false ) ) {
        	
        	mediaPolicies.add( MediaPolicy.HYBRID.name() );
            mediaPolicies.add( MediaPolicy.HDD.name() );

        }
        
        JSONObject capabilities = new JSONObject(
                new SystemCapability( mediaPolicies.toArray( new String[mediaPolicies.size()] ) ) );
        
        JSONObject toggleObject = new JSONObject();
        
        if ( FdsFeatureToggles.STATS_SERVICE_QUERY.isActive() ){
        	toggleObject.put( FdsFeatureToggles.STATS_SERVICE_QUERY.fdsname(), Boolean.TRUE );
        }
        else {
        	toggleObject.put( FdsFeatureToggles.STATS_SERVICE_QUERY.fdsname(), Boolean.FALSE );
        }
        
        capabilities.put( TOGGLES, toggleObject );

        return new JsonResource( capabilities );
    }
}
