package com.formationds.om.webkit.rest.v07.platform;

import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import com.formationds.apis.ConfigurationService;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class AddService implements RequestHandler {

    private static final Logger logger =
            LogManager.getLogger( AddService.class );

    private ConfigurationService.Iface client;
	
	public AddService( final ConfigurationService.Iface client ){
		
		this.client = client;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

        return new JsonResource( new JSONObject().put( "status", 200 ), 200);
	}

}
