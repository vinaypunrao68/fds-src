/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_GetVolInfoReqType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;

import com.formationds.apis.AmService;
import com.formationds.apis.Tenant;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.VolumeType;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.JsonArrayCollector;
import com.formationds.util.Size;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
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

    private AmService.Iface amApi;
    private ConfigurationApi config;
    private FDSP_ConfigPathReq.Iface legacyConfig;
    private AuthenticationToken token;
    private Authorizer authorizer;

    private static DecimalFormat df = new DecimalFormat("#.00");

    public ListVolumes( Authorizer authorizer, ConfigurationApi config,  AmService.Iface amApi, FDSP_ConfigPathReq.Iface legacyConfig, AuthenticationToken token ) {
        this.config = config;
        this.amApi = amApi;
        this.legacyConfig = legacyConfig;
        this.token = token;
        this.authorizer = authorizer;
    }

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = ""; // not yet supported
        JSONArray jsonArray = config.listVolumes(domain)
                                    .stream()
                                    .filter(v -> authorizer.hasAccess(token, v.getName()))
                                    .map(v -> {
                                        try {
                                            JSONObject volResponse = toJsonObject(v);

                                            // putting the tenant information here because the static call
                                            // below won't always have access to the api.  It should be injected
                                            // so that it's always acceptable
                                            List<Tenant> tenants =
                                                SingletonConfigAPI.instance().api().listTenants(0).stream()
                                                                  .filter((t) -> {

																	  return t.getId() == v.getTenantId();

																  })
                                                                  .collect(Collectors.toList());

                                            String tenantName = "";

                                            if (tenants.size() > 0) {
                                                tenantName = tenants.get(0).getIdentifier();
                                            }

                                            volResponse.put("tenant_name", tenantName);

                                            return volResponse;
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

			// if the status is null, or empty, we check the values through the AM
			// to make sure it's not just the absence of statistical reporting
			if ( status == null || ( status.getCurrentUsageInBytes() == 0 ) ) {
				try {
					status = amApi.volumeStatus("", v.getName());
				} catch( TException e ) {
					LOG.warn( "Getting Volume Status Failed", e );
				}
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
										   .withName( v.getName() )
										   .build();

		final EnumMap<FirebreakType, VolumeDatapointPair> fbResults = getFirebreakEvents( volume );

		if( v != null ) {
			o.put( "name", v.getName() );
			o.put( "volId", v.getVolId() );
			o.put( "state", v.getState() );
			o.put( "dateCreated", v.getDateCreated() );
			o.put( "tenantId", v.getTenantId() );

			if( volInfo != null ) {
				o.put( "id", Long.toString( volInfo.getVolUUID() ) );
				o.put( "priority", volInfo.getRel_prio() );
				o.put( "sla", volInfo.getIops_guarantee() );
				o.put( "limit", volInfo.getIops_max() );
				o.put( "commit_log_retention", volInfo.getContCommitlogRetention() );
			}

			if( v.getPolicy() != null ) {

				VolumeSettings p = v.getPolicy();
				JSONObject policyObj = new JSONObject();
				policyObj.put( "maxObjectSizeInBytes", p.getMaxObjectSizeInBytes() );
				policyObj.put( "volumeType", p.getVolumeType() );
				policyObj.put( "blockDeviceSizeInBytes", p.getBlockDeviceSizeInBytes() );
				policyObj.put( "contCommitLogRetention", p.getContCommitlogRetention() );
				policyObj.put( "mediaPolicy", p.getMediaPolicy() );
				o.put( "policy", policyObj );

				// NOTE: this duplicates some of the information included in the policy (VolumeSettings)
				// attached to the JSON above, which was necessary for satisfying the VolumeDescriptor sent
				// to the XDI/AM.  The UI currently uses this additional data in the display.
				// TODO: update UI to use the policy object above and remove the duplicate data
				if (v.getPolicy().getVolumeType() != null &&
					v.getPolicy().getVolumeType().equals( VolumeType.OBJECT ) ) {
					o.put( "data_connector",
						   new JSONObject().put( "type", "object" )
										   .put( "api", "S3, Swift" ) );
				} else {
					JSONObject connector = new JSONObject().put( "type", "block" );
					Size size = Size.size( v.getPolicy()
											.getBlockDeviceSizeInBytes() );
                    JSONObject attributes = new JSONObject()
                                                .put( "size", size.getCount() )
                                                .put( "unit", size.getSizeUnit().toString() );
					connector.put( "attributes", attributes );
					o.put( "data_connector", connector );
				}

				o.put( "mediaPolicy", v.getPolicy().getMediaPolicy().name() );
			}

			JSONObject fbObject = new JSONObject();
			fbObject.put( FirebreakType.CAPACITY.name().toLowerCase(),  0.0D );
			fbObject.put( FirebreakType.PERFORMANCE.name().toLowerCase(),  0.0D );

			if ( fbResults != null ){
				// put each firebreak event in the JSON object if it exists
				fbResults.forEach( (type, pair) -> {
					fbObject.put( type.name(), pair.getDatapoint().getY() );
				});
			}

			o.put( "firebreak", fbObject );
		}

		if( status != null ) {
			Size usage = Size.size( status.getCurrentUsageInBytes() );
			JSONObject dataUsage = new JSONObject()
			.put( "size", formatSize( usage ) )
			.put( "unit", usage.getSizeUnit().toString() );
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
		query.setContexts( Arrays.asList(v) );
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
			 LOG.warn( "Could not determine the firebreak events for volume: " + v.getId() + ":" + v.getName(), e );
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
