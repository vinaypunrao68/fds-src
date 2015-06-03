/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.apis.ConfigurationService;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class AddService implements RequestHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( AddService.class );

    private ConfigurationApi client;
	
	public AddService( final ConfigurationApi client ){
		
		this.client = client;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

        return new JsonResource( new JSONObject().put( "status", 200 ), 200);
	}

}
