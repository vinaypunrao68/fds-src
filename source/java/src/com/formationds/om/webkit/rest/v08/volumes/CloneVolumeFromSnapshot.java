/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.model.Snapshot;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.webkit.rest.v08.snapshots.GetSnapshot;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

import java.io.InputStreamReader;
import java.util.Map;

public class CloneVolumeFromSnapshot implements RequestHandler {

	private static final String VOLUME_ARG = "volume_id";
	private static final String SNAPSHOT_ARG = "snapshot_id";
	
	private ConfigurationApi configApi;
	private Authorizer authorizer;
	private AuthenticationToken token;

	public CloneVolumeFromSnapshot( Authorizer authorizer, AuthenticationToken token ) {
		this.authorizer = authorizer;
		this.token = token;
	}

	@Override
	public Resource handle(final Request request,
			final Map<String, String> routeParameters) throws Exception {

		long volumeId = requiredLong( routeParameters, VOLUME_ARG );
		long snapshotId = requiredLong( routeParameters, SNAPSHOT_ARG );
		
		final InputStreamReader reader = new InputStreamReader( request.getInputStream() );
		Volume newVolume = ObjectModelHelper.toObject( reader, Volume.class );
		
		Snapshot snapshot = (new GetSnapshot()).findSnapshot( snapshotId );
		
		(new CloneVolumeFromTime( getAuthorizer(), getToken() )).cloneFromTime( volumeId, newVolume, snapshot.getCreationTime().getEpochSecond() );
		
		return new TextResource("ok");
	}

	private Authorizer getAuthorizer(){
		return this.authorizer;
	}
	
	private AuthenticationToken getToken(){
		return this.token;
	}
	
	private ConfigurationApi getConfigApi() {

		if (configApi == null) {
			configApi = SingletonConfigAPI.instance().api();
		}

		return configApi;
	}
}
