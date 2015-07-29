/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.config;

import com.formationds.apis.MediaPolicy;
import com.formationds.commons.model.SystemCapability;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class SystemCapabilities
    implements RequestHandler {

    private final ParsedConfig parsedConfig;

    public SystemCapabilities( final ParsedConfig parsedConfig ) {

        this.parsedConfig = parsedConfig;

    }


    @Override
    public Resource handle( final Request request, final Map<String, String> routeParameters )
        throws Exception {

        final List<String> mediaPolicies = new ArrayList<>( );
        mediaPolicies.add( MediaPolicy.SSD_ONLY.name() );
        if( !parsedConfig.defaultBoolean( "fds.feature_toggle.common.all_ssd",
                                          false ) ) {
        	
        	mediaPolicies.add( MediaPolicy.HYBRID_ONLY.name() );
            mediaPolicies.add( MediaPolicy.HDD_ONLY.name() );

        }

        return new JsonResource(
            new JSONObject(
                new SystemCapability(
                    mediaPolicies.toArray(
                        new String[ mediaPolicies.size() ] )  ) ) );
    }
}
