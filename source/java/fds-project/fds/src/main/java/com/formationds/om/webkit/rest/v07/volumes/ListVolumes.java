/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.volumes;

import com.formationds.apis.FDSP_GetVolInfoReqType;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.Tenant;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.VolumeType;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.MediaPolicyConverter;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.protocol.FDSP_VolumeDescType;
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
import java.util.Collections;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

public class ListVolumes implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(ListVolumes.class);

    private ConfigurationApi config;
    private AuthenticationToken token;
    private Authorizer authorizer;

    private static DecimalFormat df = new DecimalFormat("#.00");

    public ListVolumes( Authorizer authorizer,
						ConfigurationApi config,
						AuthenticationToken token ) {

        this.config = config;
        this.token = token;
        this.authorizer = authorizer;

    }

	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = ""; // not yet supported
        JSONArray jsonArray = config.listVolumes(domain)
                                    .stream()
				.filter(v -> authorizer.ownsVolume(token, v.getName()))
/*
 * HACK
 * .filter( v -> isSystemVolume() != true )
 * 
 * THIS IS ALSO IN QueryHelper.java
 */
        .filter( v-> !v.getName().startsWith( "SYSTEM_VOLUME" )  )
				.map(v -> {
					try {
						JSONObject volResponse = toJsonObject(v);

						// putting the tenant information here because the static call
						// below won't always have access to the api.  It should be injected
						// so that it's always acceptable
						List<Tenant> tenants =
								SingletonConfigAPI.instance().api().listTenants(0).stream()
										.filter((t) -> t.getId() == v.getTenantId() )
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

	private JSONObject toJsonObject( VolumeDescriptor v ) throws TException {
		VolumeStatus status = null;
		FDSP_VolumeDescType volInfo = null;
		if (v != null && v.getName() != null) {
			try {
				volInfo = config.GetVolInfo(new FDSP_GetVolInfoReqType(v.getName(), 0));
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

			/*
			 * So there isn't any physical or blobs showing up in the
			 * stat stream.
			 */
			if ( status == null || ( status.getCurrentUsageInBytes() == 0 ) ) {

				/*
				 * Don't call the AM or the DM for volume status, current
				 * implementation of statVolume is VERY EXPENSIVE!
				 *
				 * setting status to zero
				 */
				status = new VolumeStatus( 0, 0 );

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

		o.put( "name", v.getName() );
		o.put( "volId", v.getVolId() );
		o.put( "state", v.getState() );
		o.put( "dateCreated", v.getDateCreated() );
		o.put( "tenantId", v.getTenantId() );

		o.put( "id", Long.toString( volInfo.getVolUUID() ) );
		o.put( "priority", volInfo.getRel_prio() );
		o.put( "sla", volInfo.getIops_assured() );
		o.put( "limit", volInfo.getIops_throttle() );

		MediaPolicy policy = MediaPolicyConverter.convertToMediaPolicy( volInfo.getMediaPolicy() );
		o.put( "mediaPolicy", policy.name() );

		o.put( "commit_log_retention", volInfo.getContCommitlogRetention() );

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
				.put( "unit", size.getSizeUnit()
								  .toString() );
                connector.put( "attributes", attributes );
                o.put( "data_connector", connector );
            }

        }

		JSONObject fbObject = new JSONObject();
		fbObject.put( FirebreakType.CAPACITY.name().toLowerCase(),  0.0D );
		fbObject.put( FirebreakType.PERFORMANCE.name().toLowerCase(),  0.0D );

		if ( fbResults != null ) {

            // put each firebreak event in the JSON object if it exists
            fbResults.forEach( (type, pair) -> fbObject.put( type.name(),
															 pair.getDatapoint()
																 .getY() ) );

        }

		o.put( "firebreak", fbObject );

		if( status != null ) {

			Size usage = Size.size( status.getCurrentUsageInBytes() );
			JSONObject dataUsage =
				new JSONObject().put( "size", formatSize( usage ) )
								.put( "unit", usage.getSizeUnit()
												   .toString() );

			o.put( "current_usage", dataUsage );
		}

		return o;
	}

	/**
	 * Getting the possible firebreak events for this volume in the past 24 hours
	 * 
	 * @param v the {@link Volume}
	 * @return Returns EnumMap<FirebreakType, VolumeDatapointPair>
	 */
	private static EnumMap<FirebreakType, VolumeDatapointPair> getFirebreakEvents( Volume v ){

		MetricQueryCriteria query = new MetricQueryCriteria();
        DateRange range = DateRange.last24Hours();

		query.setSeriesType( new ArrayList<>( Metrics.FIREBREAK ) );

		// "For now" code to use the new volume instead of the old.
		com.formationds.client.v08.model.Volume newVolume = new com.formationds.client.v08.model.Volume( 
				Long.parseLong( v.getId() ), v.getName() );

        query.setContexts( Collections.singletonList( newVolume ) );
		query.setRange( range );

		MetricRepository repo = SingletonRepositoryManager.instance().getMetricsRepository();

		final List<? extends IVolumeDatapoint> queryResults = repo.query( query );

		FirebreakHelper fbh = new FirebreakHelper();
		EnumMap<FirebreakType, VolumeDatapointPair> map = null;

		try {
			map = fbh.findFirebreakEvents( queryResults ).get( v.getId() );
        } catch ( Exception e ) {
            LOG.warn( "Could not determine the firebreak events for volume: " +
						   v.getId() + ":" + v.getName(), e );
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
