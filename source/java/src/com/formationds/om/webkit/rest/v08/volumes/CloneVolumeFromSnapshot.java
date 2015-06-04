/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CloneVolumeFromSnapshot implements RequestHandler {

	private static final String VOLUME_ARG = "volume_id";
	private static final String SNAPSHOT_ARG = "snapshot_id";
	
	private ConfigurationApi configApi;

	public CloneVolumeFromSnapshot() {
	}

	@Override
	public Resource handle(final Request request,
			final Map<String, String> routeParameters) throws Exception {

		return new TextResource("ok");
	}

	private ConfigurationApi getConfigApi() {

		if (configApi == null) {
			configApi = SingletonConfigAPI.instance().api();
		}

		return configApi;
	}
}
