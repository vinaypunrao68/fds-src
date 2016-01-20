/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.domain;

import com.formationds.client.v08.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

public class CreateLocalDomain implements RequestHandler {

	private static final Logger logger =
			LoggerFactory.getLogger( CreateLocalDomain.class );

	private ConfigurationApi configApi;

	public CreateLocalDomain( final ConfigurationApi configApi, final AuthenticationToken token ) {
		this.configApi = configApi;
	}

	@Override
	public Resource handle( Request request, Map<String, String> routeParameters )
			throws Exception {

	    Domain domain = null;
        HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );
        try ( final InputStreamReader reader = new InputStreamReader( requestLoggingProxy.getInputStream(), "UTF-8" ) ) {
            domain = ObjectModelHelper.toObject( reader, Domain.class );
        }

		logger.debug( "Creating local domain {} at site {}.", domain.getName(), domain.getSite() );

		long domainId = -1;

		try {
			domainId = getConfigApi().createLocalDomain( domain.getName(), domain.getSite() );
		} catch( ApiException e ) {

			logger.error( "POST::FAILED::" + e.getMessage(), e );

			// allow dispatcher to handle
			throw e;

		} catch ( TException | SecurityException se ) {

			logger.error( "POST::FAILED::" + se.getMessage(), se );

			// allow dispatcher to handle
			throw se;
		}

		Domain newDomain = (new GetLocalDomain()).getDomain( domainId );

		String jsonString = ObjectModelHelper.toJSON( newDomain );

		return new TextResource( jsonString );
	}

	private ConfigurationApi getConfigApi(){

		if ( configApi == null ){

			configApi = SingletonConfigAPI.instance().api();
		}

		return configApi;
	}
}

