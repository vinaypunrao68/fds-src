/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.stream;

import com.formationds.commons.model.Stream;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeregisterStream implements RequestHandler {
	
	private ConfigurationApi configApi;

	public DeregisterStream() {
	}

	@Override
	public Resource handle( Request request, Map<String, String> routeParameters )
			throws Exception {
		String source = IOUtils.toString( request.getInputStream() );
		final Stream stream = ObjectModelHelper.toObject( source, Stream.class );

		getConfigApi().deregisterStream( stream.getId() );
		return new JsonResource(new JSONObject().put("status", "OK"));
	}
	
	private ConfigurationApi getConfigApi(){
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
