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
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class ListSnapshots implements RequestHandler {

	private static final String REQ_PARAM_VOLUME_ID = "volumeId";
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
		
		List<com.formationds.protocol.Snapshot> internalSnapshots = getConfigApi().listSnapshots( volumeId );
		
		List<Snapshot> externalSnapshots = Collections.emptyList();
		
		internalSnapshots.stream().forEach( internalSnapshot -> {
			Snapshot externalSnapshot = ExternalModelConverter.convertToExternalSnapshot( internalSnapshot );
			externalSnapshots.add( externalSnapshot );
		});
		
		return externalSnapshots;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}
}
