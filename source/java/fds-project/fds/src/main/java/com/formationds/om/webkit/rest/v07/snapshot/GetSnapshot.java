package com.formationds.om.webkit.rest.v07.snapshot;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.commons.model.Snapshot;
import com.formationds.iodriver.endpoints.HttpException;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * REST endpoint to retrieve a specific snapshot by its ID.  This should be much simpler when
 * we have cycles to add this to the ConfigurationApi
 * 
 * @author nate
 *
 */
public class GetSnapshot
	implements RequestHandler{

	private static final Logger LOG = Logger.getLogger(GetSnapshot.class);
	private final static String REQ_PARAM_SNAPSHOT_ID = "snapshotId";
	
	private ConfigurationApi config;
	
	public GetSnapshot( ConfigurationApi config ){
		
		this.config = config;
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
	    final long snapshotId = requiredLong(routeParameters, REQ_PARAM_SNAPSHOT_ID);
	    LOG.debug( "Looking for a snapshot with ID: " + snapshotId );
		
		/**
		 * There is no easy way to do this at this time... there should be...
		 * So instead we have to get each volume, get each volumes snapshots
		 * and find the one that matchs.  This should be a straight query.
		 * 
		 * TODO: Extend configuration api to handle this
		 */
		
		String domain = ""; // not yet implemented
		List<VolumeDescriptor> volumes = config.listVolumes( domain );
		com.formationds.protocol.Snapshot snapshot = null;
		
		for( VolumeDescriptor volume : volumes ) {
			
			try {
				
				List<com.formationds.protocol.Snapshot> snapshots = config.listSnapshots( volume.getVolId() );
				
				Optional<com.formationds.protocol.Snapshot> snapshotMatch = snapshots.stream().filter( s -> {
					return (s.getSnapshotId() == snapshotId);
				}).findFirst();
				
				if ( snapshotMatch.isPresent() ){
					snapshot = snapshotMatch.get();
					break;
				}
				
			} catch (Exception e) {
				
				if ( volume != null ){
					LOG.error( "Problem listing snapshots for volume: " + volume.getName() + " (" + volume.getVolId() + ")", e );
				}
				else {
					LOG.error( "Trying to list snapshots for a null volume.", e );
				}
				
				LOG.error( "Skipping..." );
			}
		}// for each volume
		
		if ( snapshot != null ){
			LOG.debug( "Snapshot found." );
			Snapshot mSnapshot = new Snapshot();
			mSnapshot.setCreation( new Date( snapshot.getCreationTimestamp() ) );
			mSnapshot.setId( Long.toString( snapshot.getSnapshotId() ) );
			mSnapshot.setName( snapshot.getSnapshotName() );
			mSnapshot.setRetention( snapshot.getRetentionTimeSeconds() );
			mSnapshot.setTimelineTime( snapshot.getTimelineTime() );
			mSnapshot.setVolumeId( Long.toString( snapshot.getVolumeId() ) );
			
			String jsonString = mSnapshot.toJSON();
			return new TextResource( jsonString );
		}
		
		LOG.error( "Could not find snapshot with ID: " + snapshotId );
		throw new HttpException( HttpServletResponse.SC_BAD_REQUEST,
		                         request.getUri().toString(),
		                         request.getMethod(),
		                         "No snapshot was found with the ID: " + snapshotId );
	}

}
