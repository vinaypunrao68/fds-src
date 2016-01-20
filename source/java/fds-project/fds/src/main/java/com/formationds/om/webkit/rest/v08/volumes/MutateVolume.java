/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeSettingsISCSI;
import com.formationds.client.v08.model.VolumeSettingsNfs;
import com.formationds.commons.model.helper.ObjectModelHelper;
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

public class MutateVolume implements RequestHandler{

	private static final Logger logger = LoggerFactory
			.getLogger(MutateVolume.class);
	private static final String VOLUME_ARG = "volume_id";

	private Authorizer authorizer;
	private AuthenticationToken token;

	public MutateVolume( Authorizer authorizer, AuthenticationToken token){

		this.authorizer = authorizer;
		this.token = token;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		long volumeId = requiredLong( routeParameters, VOLUME_ARG );

		logger.debug( "Editing volume: {}.", volumeId );

		Volume volume = null;

        HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );
        try ( final InputStreamReader reader = new InputStreamReader( requestLoggingProxy.getInputStream() ) ) {
            volume = ObjectModelHelper.toObject( reader, Volume.class );
        }

		volume.setId( volumeId );

		logger.trace( ObjectModelHelper.toJSON( volume ) );

		// change the QOS
		(new CreateVolume( getAuthorizer(), getToken() )).setQosForVolume( volume );

		MutateSnapshotPolicy mutateEndpoint = new MutateSnapshotPolicy();

		// modify the snapshot policies
		volume.getDataProtectionPolicy().getSnapshotPolicies().stream().forEach( (snapshotPolicy) -> {
			try {
				mutateEndpoint.mutatePolicy( snapshotPolicy );
			} catch (Exception e) {
				logger.warn( "Could not edit snapshot policy: " + snapshotPolicy.getName(), e  );
			}
		});

		switch( volume.getSettings().getVolumeType() )
		{
			case ISCSI:
                final VolumeSettingsISCSI iscsi = ( VolumeSettingsISCSI ) volume.getSettings( );
                logger.trace( "iSCSI:: LUNS [ {} ] INITIATORS [ {} ] INCOMING USERS [ {} ] OUTGOING USERS [ {} ]",
                              iscsi.getTarget().getLuns().size(),
                              iscsi.getTarget().getInitiators().size(),
                              iscsi.getTarget().getIncomingUsers().size(),
                              iscsi.getTarget().getOutgoingUsers().size() );

                break;
			case NFS:
                final VolumeSettingsNfs nfs = ( VolumeSettingsNfs ) volume.getSettings();
                logger.trace( "NFS:: CLIENT [ {} ] OPTIONS [ {} ]", nfs.getClients(), nfs.getOptions() );
				break;
		}

		Volume newVolume = (new GetVolume( getAuthorizer(), getToken() ) ).getVolume( volumeId );

		String jsonString = ObjectModelHelper.toJSON( newVolume );

		return new TextResource( jsonString );
	}

	private Authorizer getAuthorizer(){
		return this.authorizer;
	}

	private AuthenticationToken getToken(){
		return this.token;
	}

}
