/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.snapshots;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Snapshot;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;

import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * REST endpoint to retrieve a specific snapshot by its ID. This should be much
 * simpler when we have cycles to add this to the ConfigurationApi
 * 
 * @author nate
 *
 */
public class GetSnapshot implements RequestHandler {

	private static final Logger LOG = Logger.getLogger(GetSnapshot.class);
	private final static String REQ_PARAM_SNAPSHOT_ID = "snapshot_id";

	private ConfigurationApi configApi;

	public GetSnapshot() {}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {

		final long snapshotId = requiredLong(routeParameters,
				REQ_PARAM_SNAPSHOT_ID);
		LOG.debug("Looking for a snapshot with ID: " + snapshotId);

		Snapshot snapshot = findSnapshot( snapshotId );
		
		if (snapshot != null) {
			LOG.debug("Snapshot found.");

			String jsonString = ObjectModelHelper.toJSON( snapshot );

			return new TextResource(jsonString);
		}

		LOG.error("Could not find snapshot with ID: " + snapshotId);
		throw new ApiException( "No snapshot was found with the ID: " + snapshotId, ErrorCode.MISSING_RESOURCE );
	}
	
	public Snapshot findSnapshot( long snapshotId ) throws ApiException, TException{
		/**
		 * There is no easy way to do this at this time... there should be... So
		 * instead we have to get each volume, get each volumes snapshots and
		 * find the one that matchs. This should be a straight query.
		 * 
		 * TODO: Extend configuration api to handle this
		 */

		String domain = ""; // not yet implemented
		List<VolumeDescriptor> volumes = getConfigApi().listVolumes(domain);
		com.formationds.protocol.svc.types.Snapshot snapshot = null;

		for (VolumeDescriptor volume : volumes) {

			try {

				List<com.formationds.protocol.svc.types.Snapshot> snapshots = 
						getConfigApi().listSnapshots(volume.getVolId());

				Optional<com.formationds.protocol.svc.types.Snapshot> snapshotMatch = snapshots
						.stream().filter(s -> {
							return (s.getSnapshotId() == snapshotId);
						}).findFirst();

				if (snapshotMatch.isPresent()) {
					snapshot = snapshotMatch.get();
					break;
				}

			} catch (Exception e) {

				if (volume != null) {
					LOG.error(
							"Problem listing snapshots for volume: "
									+ volume.getName() + " ("
									+ volume.getVolId() + ")", e);
				} else {
					LOG.error("Trying to list snapshots for a null volume.", e);
				}

				LOG.error("Skipping...");
			}
		}// for each volume

		if ( snapshot == null ){
			throw new ApiException( "Could not locate snapshot for the ID: " + snapshotId, ErrorCode.MISSING_RESOURCE );
		}
		
		Snapshot externalSnapshot = ExternalModelConverter.convertToExternalSnapshot( snapshot );
		
		return externalSnapshot;
	}
	
	private ConfigurationApi getConfigApi(){
		
		if ( configApi == null ){
			configApi = SingletonConfigAPI.instance().api();
		}
		
		return configApi;
	}

}
