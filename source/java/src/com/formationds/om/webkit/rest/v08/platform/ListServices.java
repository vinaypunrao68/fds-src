package com.formationds.om.webkit.rest.v08.platform;

import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class ListServices implements RequestHandler{

	private final ConfigurationApi configApi;
	
	public ListServices( final ConfigurationApi configApi ){
		this.configApi = configApi;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		// TODO Auto-generated method stub
		return null;
	}

}
