/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.model.Snapshot;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.webkit.rest.v08.snapshots.GetSnapshot;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

public class CloneVolumeFromSnapshot implements RequestHandler {

	private static final Logger logger = LoggerFactory.getLogger( CloneVolumeFromSnapshot.class );
	private static final String VOLUME_ARG = "volume_id";
	private static final String SNAPSHOT_ARG = "snapshot_id";

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

		logger.debug( "Cloning volume: {} from snapshot: {}.", volumeId, snapshotId );

		Volume newVolume = null;
        HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );
		try (final InputStreamReader reader = new InputStreamReader( requestLoggingProxy.getInputStream() )) {
		    newVolume = ObjectModelHelper.toObject( reader, Volume.class );
		}

		Snapshot snapshot = (new GetSnapshot()).findSnapshot( snapshotId );

		logger.debug( "Snapshot time was {}(sec), using that to instigate cloning operation.", snapshot.getCreationTime().getEpochSecond() );

		CloneVolumeFromTime cloneEndpoint = new CloneVolumeFromTime( getAuthorizer(), getToken() );
		Volume clonedVolume = cloneEndpoint.cloneFromTime( volumeId, newVolume, snapshot.getCreationTime().getEpochSecond() );

		String jVolume = ObjectModelHelper.toJSON( clonedVolume );

		return new TextResource( jVolume );
	}

	private Authorizer getAuthorizer(){
		return this.authorizer;
	}

	private AuthenticationToken getToken(){
		return this.token;
	}
}
