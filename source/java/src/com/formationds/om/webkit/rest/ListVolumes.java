/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;

import com.formationds.apis.AmService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.VolumeType;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.om.repository.query.builder.MetricQueryCriteriaBuilder;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.JsonArrayCollector;
import com.formationds.util.Size;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;

import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class ListVolumes implements RequestHandler {
	private static final Logger LOG = Logger.getLogger(ListVolumes.class);

	private Xdi xdi;
	private AmService.Iface amApi;
	private FDSP_ConfigPathReq.Iface legacyConfig;
	private AuthenticationToken token;

	private static DecimalFormat df = new DecimalFormat("#.00");

	public ListVolumes( Xdi xdi, AmService.Iface amApi, FDSP_ConfigPathReq.Iface legacyConfig, AuthenticationToken token ) {
		this.xdi = xdi;
		this.amApi = amApi;
		this.legacyConfig = legacyConfig;
		this.token = token;
	}

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
		JSONArray jsonArray = xdi.listVolumes(token, "")
				.stream()
				.map(v -> {
					try {
						return toJsonObject(v);
					} catch (TException e) {
						LOG.error("Error fetching configuration data for volume", e);
						throw new RuntimeException(e);
					}
				})
				.collect(new JsonArrayCollector());

		return new JsonResource(jsonArray);
	}
	/*
struct VolumeSettings {
       1: required i32 maxObjectSizeInBytes,
       2: required VolumeType volumeType,
       3: required i64 blockDeviceSizeInBytes
}

struct VolumeDescriptor {
       1: required string name,
       2: required i64 dateCreated,
       3: required VolumeSettings policy,
       4: required i64 tenantId    // Added for multi-tenancy
}
	 */

	private JSONObject toJsonObject( VolumeDescriptor v ) throws TException {
		VolumeStatus status = null;
		FDSP_VolumeDescType volInfo = null;
		if (v != null && v.getName() != null) {
			try {
				volInfo = legacyConfig.GetVolInfo(new FDSP_MsgHdrType(),
						new FDSP_GetVolInfoReqType(v.getName(), 0));
			} catch (TException e) {
				LOG.warn("Getting Volume Info Failed", e);
			}

			final Optional<VolumeStatus> optionalStatus =
					SingletonRepositoryManager.instance()
					.getMetricsRepository()
					.getLatestVolumeStatus( v.getName() );
			if( optionalStatus.isPresent() ) {

				status = optionalStatus.get();

			}
		}

		return toJsonObject( v, volInfo, status );
	}

	public static JSONObject toJsonObject(VolumeDescriptor v,
			FDSP_VolumeDescType volInfo,
			VolumeStatus status) {
		JSONObject o = new JSONObject();

		// getting firebreak information
		Volume volume = new VolumeBuilder().withId( Long.toString( volInfo.getVolUUID() ) )
				.withName( v.getName() ).build();
		
		final EnumMap<FirebreakType, VolumeDatapointPair> fbResults = getFirebreakEvents( volume );

		if( v != null ) {
			o.put( "name", v.getName() );

			if( volInfo != null ) {
				o.put( "id", Long.toString( volInfo.getVolUUID() ) );
				o.put( "priority", volInfo.getRel_prio() );
				o.put( "sla", volInfo.getIops_min() );
				o.put( "limit", volInfo.getIops_max() );
				o.put( "commit_log_retention", volInfo.getContCommitlogRetention() );
			}

			if( v.getPolicy() != null && v.getPolicy().getVolumeType() != null ) {
				if( v.getPolicy()
						.getVolumeType()
						.equals( VolumeType.OBJECT ) ) {
					o.put( "data_connector", new JSONObject().put( "type", "object" )
							.put( "api", "S3, Swift" ) );
				} else {
					JSONObject connector = new JSONObject().put( "type", "block" );
					Size size = Size.size( v.getPolicy()
							.getBlockDeviceSizeInBytes() );
					JSONObject attributes = new JSONObject()
					.put( "size", size.getCount() )
					.put( "unit", size.getSizeUnit()
							.toString() );
					connector.put( "attributes", attributes );
					o.put( "data_connector", connector );
				}

				o.put( "mediaPolicy", v.getPolicy().getMediaPolicy().name() );
			}
			
			if ( fbResults != null ){
				// put each firebreak event in the JSON object if it exists
				fbResults.forEach( (type, pair) -> {
					o.put( type.name(), pair.getDatapoint().getY() );
				});
			}
		}

		if( status != null ) {
			Size usage = Size.size( status.getCurrentUsageInBytes() );
			JSONObject dataUsage = new JSONObject()
			.put( "size", formatSize( usage ) )
			.put( "unit", usage.getSizeUnit()
					.toString() );
			o.put( "current_usage", dataUsage );
		}

		return o;
	}
	
	/**
	 * Getting the possible firebreak events for this volume in the past 24 hours
	 * 
	 * @param v
	 * @return
	 */
	private static EnumMap<FirebreakType, VolumeDatapointPair> getFirebreakEvents( Volume v ){
		
		MetricQueryCriteria query = new MetricQueryCriteria();      
		DateRange range = new DateRange();
		range.setEnd( new Date().getTime() );
		range.setStart( range.getEnd() - TimeUnit.DAYS.toMillis( 1 ) );

		query.setSeriesType( new ArrayList<Metrics>( Metrics.FIREBREAK ) );
		query.setContexts( Arrays.asList( v ) );
		query.setRange( range );

		MetricsRepository repo = SingletonRepositoryManager.instance().getMetricsRepository();

		final List<VolumeDatapoint> queryResults =
			new MetricCriteriaQueryBuilder( repo.newEntityManager() ) 
				.searchFor( query )
				.resultsList();
		
		FirebreakHelper fbh = new FirebreakHelper();
		EnumMap<FirebreakType, VolumeDatapointPair> map = null;
		
		try {
			map = fbh.findFirebreakEvents( queryResults ).get( v.getId() );
		} catch (TException e) {
			 LOG.warn( "Could not determnie the firebreak events for volume: " + v.getId() + ":" + v.getName(), e );
		}
		
		return map;
	}

	private static String formatSize(Size usage) {
		if (usage.getCount() == 0) {
			return "0";
		}

		return df.format(usage.getCount());
	}
}
