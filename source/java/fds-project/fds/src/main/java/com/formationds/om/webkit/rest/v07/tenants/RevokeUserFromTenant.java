/**
 * Copyright (c) 2015 Formation Data Systems. All rights reserved.
 */
package com.formationds.om.webkit.rest.v07.tenants;

import java.util.Map;

import javax.crypto.SecretKey;

import com.formationds.util.thrift.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

public class RevokeUserFromTenant implements RequestHandler{

	    private ConfigurationApi configCache;
		private SecretKey                                    secretKey;

	public RevokeUserFromTenant(ConfigurationApi configCache, SecretKey secretKey) {
		this.configCache = configCache;
		this.secretKey = secretKey;
	}

	@Override
	public Resource handle(Request request,
						   Map<String, String> routeParameters) throws Exception {

		long tenantId = requiredLong(routeParameters, "tenantid");
		long userId = requiredLong(routeParameters, "userid");
		configCache.revokeUserFromTenant(userId, tenantId);
		return new JsonResource(new JSONObject().put("status", "ok"));
		}
}
