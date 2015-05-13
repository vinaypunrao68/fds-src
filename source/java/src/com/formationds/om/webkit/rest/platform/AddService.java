package com.formationds.om.webkit.rest.platform;

import java.util.Map;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class AddService implements RequestHandler {

    private static final Logger logger =
            LoggerFactory.getLogger( AddService.class );

    private FDSP_ConfigPathReq.Iface client;
	
	public AddService( final FDSP_ConfigPathReq.Iface client ){
		
		this.client = client;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

        return new JsonResource( new JSONObject().put( "status", 200 ), 200);
	}

}
