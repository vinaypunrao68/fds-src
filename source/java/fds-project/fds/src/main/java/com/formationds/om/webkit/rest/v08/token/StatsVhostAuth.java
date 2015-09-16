package com.formationds.om.webkit.rest.v08.token;

import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class StatsVhostAuth implements RequestHandler {

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		return new TextResource( "allow" );
	}

}
