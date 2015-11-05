/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.domain;

import com.formationds.apis.LocalDomainDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class ListLocalDomains implements RequestHandler {
	private static final Logger logger = LoggerFactory
			.getLogger(ListLocalDomains.class);

	private ConfigurationApi configApi;

	public ListLocalDomains(){}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		List<Domain> localDomains = Collections.emptyList();
		
		try {
			localDomains = listDomains();
		} catch (Exception e) {
			logger.error("GET::FAILED::" + e.getMessage(), e);

			// allow dispatcher to handle
			throw e;
		}

		String jsonString = ObjectModelHelper.toJSON( localDomains );

		return new TextResource(jsonString);
	}
	
	public List<Domain> listDomains() throws Exception{
		
		logger.debug("Listing local domains.");
		List<LocalDomainDescriptor> internalDomains = getConfigApi().listLocalDomains( 0 );

		List<Domain> externalDomains = new ArrayList<Domain>();
		
		internalDomains.stream().forEach( internalDomain -> {
			
			Domain externalDomain = ExternalModelConverter.convertToExternalDomain( internalDomain );
			externalDomains.add( externalDomain );
		});
		
		return externalDomains;
	}

	private ConfigurationApi getConfigApi() {

		if (configApi == null) {
			configApi = SingletonConfigAPI.instance().api();
		}

		return configApi;
	}
}
