/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class CreateSnapshotPolicy implements RequestHandler{
	
	private final ConfigurationApi config;
	
	public CreateSnapshotPolicy( final ConfigurationApi config ){
		
		this.config = config;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		// TODO Auto-generated method stub
		return null;
	}

}
