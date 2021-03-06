/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.flag;

import com.formationds.commons.togglz.feature.FdsFeatureManagerProvider;
import org.togglz.core.Feature;
import org.togglz.core.annotation.Label;
import org.togglz.core.repository.FeatureState;

/**
 * feature togglz annotation
 *
 * Togglz annotation are used to group features together in "feature groups".
 *
 * @author ptinius
 */
public enum FdsFeatureToggles implements Feature {

    @Label( "OM Service Proxy Feature" )
    OM_SERVICE_PROXY("fds.feature_toggle.om.enable_java_om_svc_proxy"),

    @Label( "OM Config Rest Client Proxy use v08 API" )
    OM_CONFIG_REST_CLIENT_PROXY_V08( "fds.feature_toggle.om.configsvc_rest_client_proxy_v08" ),

    @Label( "REST 0.7 API Implementation" )
    REST_07( "fds.feature_toggle.om.rest_07_feature" ),

    @Label( "Enable old firebreak query behavior." )
    FS_2660_METRIC_FIREBREAK_EVENT_QUERY( "fds.feature_toggle.om.fs_2660_metric_firebreak_event_query" ),

    @Label( "Are we using the new super block?, i.e. partition scheme" )
    NEW_SUPERBLOCK( "fds.feature_toggle.pm.use_new_superblock" ),

    @Label( "Enable InfluxDB Write batching" )
    INFLUX_WRITE_BATCHING( "fds.feature_toggle.om.enable_influxdb_write_batch" ),

    @Label( "Enable InfluxDB Chunked Response Query.  Requires fork of influxdb-java")
    INFLUX_CHUNKED_QUERY_RESPONSE( "fds.feature_toggle.om.enable_influxdb_chunked_query_response" ),

    @Label( "Enable web logging request wrapper")
    WEB_LOGGING_REQUEST_WRAPPER( "fds.feature_toggle.common.enable_web_logging_request_wrapper" ),

    @Label( "Original influxdb implementation that uses a single series containing all volume metrics.")
    INFLUX_ONE_SERIES( "fds.feature_toggle.om.enable_influxdb_one_series" ),

    @Label( "Experimental influxdb implementation that uses a series per volume" )
    INFLUX_SERIES_PER_VOLUME( "fds.feature_toggle.om.enable_influxdb_series_per_volume" ),

    @Label( "If enabled, use query results from experimental series per volume influxdb schema." )
    INFLUX_QUERY_SERIES_PER_VOLUME( "fds.feature_toggle.om.enable_influxdb_query_series_per_volume" ),
    
    @Label( "If enabled, queries for stats will no longer go to Influx, but will go to the stats service instead." )
    STATS_SERVICE_QUERY( "fds.feature_toggle.om.enable_stats_service_query" ),
    
    @Label( "If enabled, the OM will write metrics into the Influx Database.")
    INFLUX_WRITE_METRICS( "fds.feature_toggle.om.enable_influx_persistence" ),

    @Label( "Enable subscriptions" )
    SUBSCRIPTIONS( "fds.feature_toggle.common.enable_subscriptions" ),

    @Label( "Enable Thrift multiplexed services on any transport, protocol, and server." )
    THRIFT_MULTIPLEXED_SERVICES( "fds.feature_toggle.common.enable_multiplexed_services" ),

    @Label( "If enabled, use a shared QueryHelper instance that caches results." )
    USE_SHARED_QUERY_HELPER( "fds.feature_toggle.om.use_shared_queryhelper" ),

    @Label( "If enabled, use a web filter that checks the domain is up before contacting config service." )
    ENABLE_DOMAIN_WEB_FILTER( "fds.feature_toggle.om.enable_domain_web_filter" ),

    @Label( "If enabled, volume grouping is n=being used." )
    USE_VOLUME_GROUPING( "fds.feature_toggle.common.enable_volumegrouping" );

    /**
     * @return Returns {@code true} if the feature associated with {@code this}
     * is enabled
     */
    public boolean isActive() {

        return FdsFeatureManagerProvider.getFeatureManager()
                                        .isActive( this );

    }

    /**
     * @param featureState the {@code boolean} flag used to set the feature availability.
     */
    public void state( final boolean featureState ) {
        FdsFeatureManagerProvider.getFeatureManager()
                                 .setFeatureState( new FeatureState( this )
                                                       .setEnabled( featureState ) );
    }

    private final String fdsName;

    FdsFeatureToggles( final String fdsName ) {
        this.fdsName = fdsName;
    }

    public String fdsname()
    {
        return this.fdsName;
    }
}
