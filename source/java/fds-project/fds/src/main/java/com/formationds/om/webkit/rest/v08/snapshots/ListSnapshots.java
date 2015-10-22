/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.snapshots;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Snapshot;
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
import java.util.List;
import java.util.Map;

public class ListSnapshots implements RequestHandler {

	private static final Logger logger = LoggerFactory.getLogger( ListSnapshots.class );
	private static final String REQ_PARAM_VOLUME_ID = "volume_id";
	private ConfigurationApi configApi;

	public ListSnapshots() {
	}

	@Override
	public Resource handle(final Request request,
			final Map<String, String> routeParameters) throws Exception {
		
		final long volumeId = requiredLong(routeParameters, REQ_PARAM_VOLUME_ID);
		
		final List<Snapshot> snapshots = listSnapshots( volumeId );
		
		String jsonString = ObjectModelHelper.toJSON( snapshots );
		
		return new TextResource( jsonString );
	}
	
	public List<Snapshot> listSnapshots( long volumeId ) throws Exception{
		
		logger.debug( "Looking for snapshots for volume: {}.", volumeId );
		
		List<com.formationds.protocol.svc.types.Snapshot> internalSnapshots = getConfigApi().listSnapshots( volumeId );
		
		List<Snapshot> externalSnapshots = new ArrayList<>();
		
		internalSnapshots.stream().forEach( internalSnapshot -> {
			Snapshot externalSnapshot = ExternalModelConverter.convertToExternalSnapshot( internalSnapshot );
			externalSnapshots.add( externalSnapshot );
		});
		
		logger.debug( "Found {} snapshots.", externalSnapshots.size() );
		
		return externalSnapshots;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
