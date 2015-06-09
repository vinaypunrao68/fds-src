/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.snapshots;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class DeleteSnapshot 	implements RequestHandler{

	private static final Logger LOG = Logger.getLogger(GetSnapshot.class);
	private final static String REQ_PARAM_SNAPSHOT_ID = "snapshotId";
	
	private ConfigurationApi config;
	
	public DeleteSnapshot( ConfigurationApi config ){
		
		this.config = config;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		return new TextResource( "ok" );
	}
}
