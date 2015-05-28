/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.apis.ConfigurationService;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class CreateVolume implements RequestHandler {

	private static final Logger logger = LoggerFactory
			.getLogger(CreateVolume.class);

	private final Authorizer authorizer;
	private final ConfigurationService.Iface configApi;
	private final AuthenticationToken token;

	public CreateVolume( final ConfigurationService.Iface configApi,
			final Authorizer authorizer,
			final AuthenticationToken token) {
		this.authorizer = authorizer;
		this.configApi = configApi;
		this.token = token;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		return new TextResource("ok");
	}
}
