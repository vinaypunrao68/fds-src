package com.formationds.om.webkit.rest.v08.domain;

import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.client.v08.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetLocalDomain implements RequestHandler{

	private static final String DOMAIN_ARG = "domain_id";
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		long domainId = requiredLong( routeParameters, DOMAIN_ARG );
		
		Domain myDomain = getDomain( domainId );
		
		if ( myDomain == null ){
			throw new ApiException( "Domain for ID: " + domainId + " was not found.", ErrorCode.MISSING_RESOURCE );
		}
		
		String jsonString = ObjectModelHelper.toJSON( myDomain );
		
		return new TextResource( jsonString );
	}
	
	public Domain getDomain( long anId ) throws Exception{
		
		List<Domain> domains = (new ListLocalDomains()).listDomains();
		
		Domain myDomain = null;
		
		for ( Domain domain : domains ){
			if ( domain.getId().equals( anId ) ){
				myDomain = domain;
				break;
			}
		}
		
		return myDomain;
	}

}
