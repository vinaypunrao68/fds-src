/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class MutateTenant implements RequestHandler{

	public MutateTenant(){}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		return new JsonResource( new JSONObject().put( "message", "Not implemented yet." ), 
				                 HttpServletResponse.SC_NOT_IMPLEMENTED );
	}

}
