package com.formationds.om.webkit.rest.v08.tenants;

import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class MutateTenant implements RequestHandler{

	private final ConfigurationApi config;
	
	public MutateTenant( final ConfigurationApi config ){
		this.config = config;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		// TODO Auto-generated method stub
		return null;
	}

}
